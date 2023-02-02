






                         CHAPTER  1


                         FRANZ LISP







1.1.  FRANZ LISP[] was created as a tool to further research
in  symbolic and algebraic manipulation, artificial intelli-
gence, and programming languages at the University of  Cali-
fornia at Berkeley.  Its roots are in the PDP-11 Lisp system
which  originally  came from Harvard.  As it grew it adopted
features of Maclisp and Lisp Machine Lisp which enables  our
work to be shared with colleagues at the Laboratory for Com-
puter Science  at  M.I.T.   Substantial  compatibility  with
other   Lisp   dialects  (Interlisp,  UCILisp,  CMULisp)  is
achieved by means of support packages and compiler switches.
FRANZ  LISP  is  written  almost entirely in the programming
language C.  A  small  part  is  written  in  the  assembler
language  for  the  current  host machine, a VAX 11/780, and
part is written in Lisp.  Because FRANZ LISP is  written  in
C, it is relatively portable and easy to comprehend.

     FRANZ LISP is capable of running large lisp programs in
a  timesharing  environment,  has  facilities for arrays and
user defined structures, has a user controlled  reader  with
character  and  word  macro   capabilities, and can interact
directly with compiled Lisp, C, Fortran, and Pascal code.

     This document is a reference manual for the FRANZ  LISP
system.   It  is  not  a  Lisp primer or introduction to the
language.  Some parts will be  of  interest  only  to  those
maintaining  FRANZ  LISP at their computer site.  This docu-
ment is divided into four Movements.  In the  first  one  we
will  attempt  to  describe  the language of FRANZ LISP pre-
cisely and completely as  it  now  stands  (Opus  38,  April
1982).   In  the second Movement we will look at the reader,
function types, arrays and exception handling.  In the third
Movement  we  will  look  at  several large support packages
written to help the FRANZ LISP user, namely the trace  pack-
age,   compiler,  fixit  and  stepping package.  Finally the
fourth movement contains  an index into the other movements.
____________________
9   []It is rumored that this name has something to  do  with
Franz  Liszt  [F_rants List] (1811-1886) a Hungarian composer
and keyboard virtuoso. These  allegations  have  never  been
proven.



9FRANZ LISP                                               1-1







FRANZ LISP                                               1-2


In the rest of this chapter we shall examine the data  types
of  FRANZ  LISP.  The conventions used in the description of
the FRANZ LISP functions will be given in section 1.4 --  it
is very important that these conventions are  understood.




1.2.  Data Types   FRANZ LISP has  eleven  data  types.   In
this  section  we shall look in detail at each type and if a
type is divisible we shall look inside it.  There is a  Lisp
function  _t_y_p_e  which  will  return  the type name of a lisp
object.  This is the official FRANZ LISP name for that  type
and  we  will use this name and this name only in the manual
to avoid confusing the reader.   The  types  are  listed  in
terms of importance rather than alphabetically.




1.2.0.  lispval   This is the name we use  to  describe  any
lisp object.  The function _t_y_p_e will never return `lispval'.




1.2.1.  symbol   This object corresponds to  a  variable  in
most  other  programming  languages.  It may have a value or
may be `unbound'.  A symbol may be _l_a_m_b_d_a _b_o_u_n_d meaning that
its  current  value  is stored away somewhere and the symbol
is given a new value for the duration of a certain  context.
When  the  Lisp processor  leaves that context, the symbol's
current value is thrown away and its old value is restored.
9A symbol may also have a _f_u_n_c_t_i_o_n  _b_i_n_d_i_n_g.   This  function
binding  is static; it cannot be lambda bound.  Whenever the
symbol is used in the functional position of a Lisp  expres-
sion  the  function  binding  of the symbol is examined (see
Chapter 4 for more details on  evaluation).
9A symbol may also have a _p_r_o_p_e_r_t_y _l_i_s_t, another static  data
structure.   The property list consists of a list of an even
number of elements, considered to be grouped as  pairs.  The
first  element  of  the pair is the _i_n_d_i_c_a_t_o_r the second the
_v_a_l_u_e of that indicator.
9Each symbol has a print name (_p_n_a_m_e) which is how this  sym-
bol  is  accessed  from  input and referred to on  (printed)
output.
9A symbol also has a hashlink used to link  symbols  together
in  the  oblist  --  this  field is inaccessible to the lisp
user.
9Symbols are created by  the  reader  and  by  the  functions


9                                 Printed: September 21, 1982







FRANZ LISP                                               1-3


_c_o_n_c_a_t,  _m_a_k_n_a_m and their derivatives.  Most symbols live on
FRANZ LISP's sole _o_b_l_i_s_t, and therefore two symbols with the
same print name are usually the  exact same object (they are
_e_q).  Symbols which are not on the oblist  are  said  to  be
_u_n_i_n_t_e_r_n_e_d.   The function _m_a_k_n_a_m creates uninterned symbols
while _c_o_n_c_a_t creates _i_n_t_e_r_n_e_d ones.


8____________________________________________________________
 Subpart name   Get value   Set value          Type

8________________________________________________________________________________________________________________________
    value         eval         set            lispval
                              setq
8____________________________________________________________
   property       plist     setplist        list or nil
     list          get       putprop
8____________________________________________________________
   function       getd        putd      array, binary, list
   binding                     def            or nil
8____________________________________________________________
  print name    get_pname                     string
8____________________________________________________________
  hash link
8____________________________________________________________
7|7|7|7|7|7|7|7|7|7|7|7|7|











             |7|7|7|7|7|7|7|7|7|7|7|7|











                         |7|7|7|7|7|7|7|7|7|7|7|7|











                                     |7|7|7|7|7|7|7|7|7|7|7|7|











                                                           |7|7|7|7|7|7|7|7|7|7|7|7|

















1.2.2.  list   A list cell has two parts, called the car and
cdr.  List cells are created by the function _c_o_n_s.


8      ________________________________________________
       Subpart name   Get value   Set value    Type

8      ________________________________________________________________________________________________
           car           car       rplaca     lispval
8      ________________________________________________
           cdr           cdr       rplacd     lispval
8      ________________________________________________
7     |8|7|7|7|7|



9                   |8|7|7|7|7|



9                               |8|7|7|7|7|



9                                           |8|7|7|7|7|



9                                                     |8|7|7|7|7|





9



1.2.3.  binary   This type acts as  a  function  header  for
machine coded functions.  It has two parts, a pointer to the
start  of  the  function  and  a  symbol  whose  print  name
describes  the  argument  _d_i_s_c_i_p_l_i_n_e.   The  discipline  (if
_l_a_m_b_d_a, _m_a_c_r_o or _n_l_a_m_b_d_a) determines whether  the  arguments
to this function will be evaluated by the caller before this
function is called.  If the discipline is a  string  (either
"_s_u_b_r_o_u_t_i_n_e",   "_f_u_n_c_t_i_o_n",  "_i_n_t_e_g_e_r-_f_u_n_c_t_i_o_n",  or  "_r_e_a_l-
_f_u_n_c_t_i_o_n") then this function is  a  foreign  subroutine  or
function  (see  8.4 for more details on this).  Although the
type of the _e_n_t_r_y field of a binary type  object  is  either


9                                 Printed: September 21, 1982







FRANZ LISP                                               1-4


string  or  fixnum,  the  object  pointed  to  is actually a
sequence of machine instructions.
Objects of type binary are created by _m_f_u_n_c_t_i_o_n.


8 _________________________________________________________
  Subpart name   Get value   Set value         Type

8 __________________________________________________________________________________________________________________
     entry       getentry                string or fixnum
8 _________________________________________________________
   discipline     getdisc     putdisc    symbol or fixnum
8 _________________________________________________________
7|8|7|7|7|7|



9              |8|7|7|7|7|



9                          |8|7|7|7|7|



9                                      |8|7|7|7|7|



9                                                         |8|7|7|7|7|





9



1.2.4.  fixnum   A fixnum is  an  integer  constant  in  the
range  -2[31] to 2[31]-1.  Small fixnums (-1024 to 1023) are
stored in a special table so they needn't be allocated  each
time one is needed.




1.2.5.  flonum   A flonum is a double precision real  number
in the range +_2.9x10[-37] to +_1.7x10[38].  There are approx-
imately sixteen decimal digits of precision.




1.2.6.  bignum   A  bignum  is  an  integer  of  potentially
unbounded  size.  When integer arithmetic exceeds the limits
mentioned above the calculation is automatically  done  with
bignums.   Should  calculation  with  bignums  give a result
which can be  represented  as  a  fixnum,  then  the  fixnum
representation will be used[].  This contraction is known as
_i_n_t_e_g_e_r  _n_o_r_m_a_l_i_z_a_t_i_o_n.   Many  Lisp  functions  assume that
integers are normalized.  Bignums are composed of a sequence
of  list cells and a cell known as an sdot.  The user should
consider a bignum structure indivisible  and  use  functions
such  as  _h_a_i_p_a_r_t,  and _b_i_g_n_u_m-_l_e_f_t_s_h_i_f_t to extract parts of
it.



____________________
9   []The current algorithms for  integer  arithmetic  opera-
tions will return (in certain cases) a result between +_2[30]
and 2[31] as a bignum although this could be represented  as
a fixnum.



9                                 Printed: September 21, 1982







FRANZ LISP                                               1-5


1.2.7.  string   A string is a null terminated  sequence  of
characters.   Most functions of symbols which operate on the
symbol's print name will also work on strings.  The  default
reader  syntax  is set so that a sequence of characters sur-
rounded by double quotes is a string.




1.2.8.  port   A port is a structure which  the  system  I/O
routines  can  reference  to  transfer data between the Lisp
system and external media.  Unlike other Lisp objects  there
are  a  very  limited number of ports (20).  Ports are allo-
cated by _i_n_f_i_l_e and _o_u_t_f_i_l_e and  deallocated  by  _c_l_o_s_e  and
_r_e_s_e_t_i_o.  The _p_r_i_n_t function prints a port as a percent sign
followed by the name of the file it is  connected  to.   The
file names $stdin, $stdout and $stderr refer to the standard
input, output and error files.




1.2.9.  array   Arrays are rather complicated types and  are
fully  described in Chapter 9.  An array consists of a block
of contiguous data, a function to access that data and auxi-
liary  fields  for  use by the accessing function.  Since an
array's accessing function is created by the user, an  array
can have any form the user chooses (e.g. n-dimensional, tri-
angular, or hash table).
Arrays are created by the function _m_a_r_r_a_y.


8_______________________________________________________________
  Subpart name     Get value   Set value          Type

8______________________________________________________________________________________________________________________________
 access function   getaccess   putaccess      binary, list
                                                or symbol
8_______________________________________________________________
    auxiliary       getaux      putaux           lispval
8_______________________________________________________________
      data         arrayref     replace    block of contiguous
                                  set            lispval
8_______________________________________________________________
     length        getlength   putlength         fixnum
8_______________________________________________________________
      delta        getdelta    putdelta          fixnum
8_______________________________________________________________
7|7|7|7|7|7|7|7|7|7|7|7|










                |7|7|7|7|7|7|7|7|7|7|7|










                            |7|7|7|7|7|7|7|7|7|7|7|










                                        |7|7|7|7|7|7|7|7|7|7|7|










                                                              |7|7|7|7|7|7|7|7|7|7|7|





















9                                 Printed: September 21, 1982







FRANZ LISP                                               1-6


1.2.10.  value   A  value  cell  contains  a  pointer  to  a
lispval.  This type is used mainly by arrays of general lisp
objects.  Value cells are created with the _p_t_r function.   A
value  cell  containing  a  pointer  to  the symbol `foo' is
printed as `(ptr to)foo'




1.2.11.  hunk   A  hunk  is  a  vector  of  from  1  to  128
lispvals.   Once  a  hunk is created (by _h_u_n_k or _m_a_k_h_u_n_k) it
cannot grow or shrink.  The access time for an element of  a
hunk  is  slower than a list cell element but faster than an
array.  Hunks are really only allocated in sizes  which  are
powers  of two, but can appear to the user to be any size in
the 1 to 128  range.   Users  of  hunks  must  realize  that
(_n_o_t (_a_t_o_m '_l_i_s_p_v_a_l)) will return true if _l_i_s_p_v_a_l is a hunk.
Most lisp systems do not have a direct test for a list  cell
and instead use the above test and assume that a true result
means _l_i_s_p_v_a_l is a list cell.  In FRANZ  LISP  you  can  use
_d_t_p_r  to check for a list cell.  Although hunks are not list
cells, you can still access the first two hunk elements with
_c_d_r and _c_a_r and you can access any hunk element with  _c_x_r[].
You  can  set  the value of the first two elements of a hunk
with _r_p_l_a_c_d and _r_p_l_a_c_a and you can set the value of any ele-
ment of the hunk with _r_p_l_a_c_x.  A hunk is printed by printing
its contents surrounded by { and }.  However a  hunk  cannot
be  read  in in this way in the standard lisp system.  It is
easy to write a reader macro to do this if desired.




1.3.  Documentation   The conventions used in the  following
chapters  were  designed to give a great deal of information
in a brief space.  The first line of a function  description
contains  the  function name in bold face and then lists the
arguments, if any.  The arguments all have names which begin
with  a letter  or letters and an underscore.  The letter(s)
gives the allowable type(s) for that argument  according  to
this table.






____________________
9   []In a hunk, the function _c_d_r references the  first  ele-
ment and _c_a_r the second.




9                                 Printed: September 21, 1982







FRANZ LISP                                               1-7


8  _______________________________________________________
   Letter                Allowable type(s)

8  ______________________________________________________________________________________________________________
   g        any type
8  _______________________________________________________
   s        symbol (although nil may not be allowed)
8  _______________________________________________________
   t        string
8  _______________________________________________________
   l        list (although nil may be allowed)
8  _______________________________________________________
   n        number (fixnum, flonum, bignum)
8  _______________________________________________________
   i        integer (fixnum, bignum)
8  _______________________________________________________
   x        fixnum
8  _______________________________________________________
   b        bignum
8  _______________________________________________________
   f        flonum
8  _______________________________________________________
   u        function type (either binary or lambda body)
8  _______________________________________________________
   y        binary
8  _______________________________________________________
   a        array
8  _______________________________________________________
   v        value
8  _______________________________________________________
   p        port (or nil)
8  _______________________________________________________
   h        hunk
8  _______________________________________________________
7 |7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|























         |7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|























                                                        |7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|7|


























In the first line of a function description, those arguments
preceded  by  a quote mark are evaluated (usually before the
function is called).  The quoting convention is used so that
we  can give a name to the result of evaluating the argument
and we can describe the allowable types.  If an argument  is
not  quoted  it does not mean that that argument will not be
evaluated, but rather that if it is evaluated, the  time  at
which  it is evaluated will be specifically mentioned in the
function description.  Optional arguments are surrounded  by
square brackets.  An ellipsis means zero or more occurrences
of an argument of the directly preceding type.
















9                                 Printed: September 21, 1982



