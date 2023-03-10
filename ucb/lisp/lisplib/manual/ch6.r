






                         CHAPTER  6


                      System Functions



This chapter describes the functions  used to interact  with
internal components of the Lisp system and operating system.

(allocate 's_type 'x_pages)

     WHERE:   s_type is one of the  FRANZ  LISP  data  types
              described in 1.3.

     RETURNS: x_pages.

     SIDE EFFECT: FRANZ LISP attempts to allocate x_pages of
                  type  s_type.   If there aren't x_pages of
                  memory left, no space  will  be  allocated
                  and an error will occur.  The storage that
                  is allocated is not given to  the  caller,
                  instead  it  is  added to the free storage
                  list of s_type.  The functions _s_e_g_m_e_n_t and
                  _s_m_a_l_l-_s_e_g_m_e_n_t  allocate blocks  of storage
                  and return it to the caller.

(argv 'x_argnumb)

     RETURNS: a symbol whose pname is the x_argnumb_t_h  argu-
              ment (starting at 0) on the command line which
              invoked the current lisp.

     NOTE: if x_argnumb is less than zero,  a  fixnum  whose
           value  is  the number of arguments on the command
           line is returned.  (_a_r_g_v _0) returns the  name  of
           the lisp you are running.

(baktrace)

     RETURNS: nil

     SIDE EFFECT: the lisp runtime stack is examined and the
                  name  of (most) of the functions currently
                  in  execution  are  printed,  most  active
                  first.

     NOTE: this will occasionally miss the names of compiled
           lisp  functions  due to incomplete information on
           the stack.  If you  are  tracing  compiled  code,
           then  _b_a_k_t_r_a_c_e  won't  be  able  to interpret the
           stack unless  (_s_s_t_a_t_u_s _t_r_a_n_s_l_i_n_k _n_i_l)  was  done.
           See  the  function  _s_h_o_w_s_t_a_c_k  for another way of


System Functions                                         6-1







System Functions                                         6-2


           printing the lisp runtime stack.

(boundp 's_name)

     RETURNS: nil  if s_name is  unbound,  that  is  it  has
              never  be  given  a  value.  If x_name has the
              value g_val, then (nil . g_val) is returned.

(chdir 's_path)

     RETURNS: t iff the system call succeeds.

     SIDE EFFECT: the current directory set to s_path. Among
                  other things, this will affect the default
                  location where the input/output  functions
                  look for and create files.

     NOTE: _c_h_d_i_r follows the standard UNIX  conventions,  if
           s_path  does  not begin with a slash, the default
           path is changed to the current path  with  s_path
           appended.

(dumplisp s_name)

     RETURNS: nil

     SIDE EFFECT: the current lisp is  dumped  to  the  disk
                  with the file name s_name.  When s_name is
                  executed, you will be in  a  lisp  in  the
                  same state as when the dumplisp was done.

     NOTE: dumplisp will fail if  one tries  to  write  over
           the current running file. UNIX does not allow you
           to modify the file you are running.

(eval-when l_time g_exp1 ...)

     SIDE EFFECT: l_time may contain any combination of  the
                  symbols  _l_o_a_d,  _e_v_a_l,  and  _c_o_m_p_i_l_e.   The
                  effects of load and compile  is  discussed
                  in  the  section on the compiler.  If eval
                  is present however, this simply means that
                  the  expressions  g_exp1  and  so  on  are
                  evaluated from left to right.  If eval  is
                  not present, the forms are not evaluated.







9

9                                    Printed: October 7, 1982







System Functions                                         6-3


(exit ['x_code])

     RETURNS: nothing (it never returns).

     SIDE EFFECT: the lisp system dies with exit code x_code
                  or 0 if x_code is not specified.

(fake 'x_addr)

     RETURNS: the lisp object at address x_addr.

     NOTE: This is intended to be used by  people  debugging
           the lisp system.

(fork)

     RETURNS: nil to  the  child  process  and  the  process
              number of the child to the parent.

     SIDE EFFECT: A copy of the current lisp system is  made
                  in  memory and both lisp systems now begin
                  to  run.   This  function  can   be   used
                  interactively   to  temporarily  save  the
                  state of Lisp (as shown  below),  but  you
                  must  be  careful  that  only  one  of the
                  lisp's interacts with the  terminal  after
                  the fork.  The _w_a_i_t function is useful for
                  this.


    ____________________________________________________

    -> (_s_e_t_q _f_o_o '_b_a_r)              ;; set a variable
    bar
    -> (_c_o_n_d ((_f_o_r_k)(_w_a_i_t)))        ;; duplicate the lisp system and
    nil                             ;; make the parent wait
    -> _f_o_o                          ;; check the value of the variable
    bar
    -> (_s_e_t_q _f_o_o '_b_a_z)              ;; give it a new value
    baz
    -> _f_o_o                          ;; make sure it worked
    baz
    -> (_e_x_i_t)                       ;; exit the child
    (5274 . 0)                      ;; the _w_a_i_t function returns this
    -> _f_o_o                          ;; we check to make sure parent was
    bar                             ;; not modified.
    ____________________________________________________





9

9                                    Printed: October 7, 1982







System Functions                                         6-4


(gc)

     RETURNS: nil

     SIDE EFFECT: this causes a garbage collection.

     NOTE: The function _g_c_a_f_t_e_r is not called  automatically
           after  this function finishes.  Normally the user
           doesn't have to call _g_c since garbage  collection
           occurs automatically whenever internal free lists
           are exhausted.

(gcafter s_type)

     WHERE:   s_type is one of the  FRANZ  LISP  data  types
              listed in 1.3.

     NOTE: this function is called by the garbage  collector
           after  a  garbage  collection which was caused by
           running out of data type s_type.   This  function
           should  determine if more space need be allocated
           and if so should allocate it.  There is a default
           gcafter  function but users who want control over
           space allocation can define their own -- but note
           that it must be an nlambda.

(getenv 's_name)

     RETURNS: a symbol whose pname is the value of s_name in
              the   current  UNIX  environment.   If  s_name
              doesn't exist in the  current  environment,  a
              symbol with a null pname is returned.

(hashtabstat)

     RETURNS: a list of fixnums representing the  number  of
              symbols in each bucket of the oblist.

     NOTE: the oblist is stored a  hash  table  of  buckets.
           Ideally there would be the same number of symbols
           in each bucket.











9

9                                    Printed: October 7, 1982







System Functions                                         6-5


(help [sx_arg])

     SIDE EFFECT: If sx_arg is a symbol then the portion  of
                  this manual beginning with the description
                  of sx_arg is printed on the terminal.   If
                  sx_arg  is  a fixnum or the name of one of
                  the appendicies, that chapter or  appendix
                  is  printed  on the terminal.  If no argu-
                  ment is provided, _h_e_l_p prints the  options
                  that it recognizes.  The program `more' is
                  used to print the manual on the  terminal;
                  it will stop after each page and will con-
                  tinue after the space key is pressed.

(include s_filename)

     RETURNS: nil

     SIDE EFFECT: The given  filename  is  _l_o_a_ded  into  the
                  lisp.

     NOTE: this is similar to load except  the  argument  is
           not  evaluated.   Include means something special
           to the compiler.

(includef 's_filename)

     RETURNS: nil

     SIDE EFFECT: this is the same  as  _i_n_c_l_u_d_e  except  the
                  argument is evaluated.

(maknum 'g_arg)

     RETURNS: the address of its argument converted  into  a
              fixnum.

(monitor ['xs_maxaddr])

     RETURNS: t

     SIDE EFFECT: If xs_maxaddr is t then profiling  of  the
                  entire   lisp   system   is   begun.    If
                  xs_maxaddr is a fixnum then  profiling  is
                  done  only  up  to address xs_maxaddr.  If
                  xs_maxaddr is not given, then profiling is
                  stopped  and  the data obtained is written
                  to the file  'mon.out'  where  it  can  be
                  analyzed with the UNIX 'prof' program.

     NOTE: this function only works if the lisp  system  has
           been compiled in  a special way.
9

9                                    Printed: October 7, 1982







System Functions                                         6-6


(opval 's_arg ['g_newval])

     RETURNS: the value associated  with  s_arg  before  the
              call.

     SIDE EFFECT: If g_newval is specified, the value  asso-
                  ciated with s_arg is changed to g_newval.

     NOTE: _o_p_v_a_l keeps track of storage allocation. If s_arg
           is one of the data types then _o_p_v_a_l will return a
           list of three fixnums representing the number  of
           items  of  that  type in use, the number of pages
           allocated and the number of items  of  that  type
           per  page.  You  should  never  try to change the
           value _o_p_v_a_l associates with  a  data  type  using
           _o_p_v_a_l.
           If s_arg is _p_a_g_e_l_i_m_i_t then _o_p_v_a_l will return (and
           set  if  g_newval is given) the maximum amount of
           lisp data pages it  will  allocate.   This  limit
           should  remain small unless you know your program
           requires lots of space as this limit  will  catch
           programs   in  infinite  loops  which  gobble  up
           memory.

(*process 'st_command ['g_readp ['g_writep]])

     RETURNS: either a fixnum if one argument is given, or a
              list of two ports and a fixnum if two or three
              arguments are given.

     NOTE: *_p_r_o_c_e_s_s  starts  another  process   by   passing
           st_command to the shell (it first tries csh, then
           it tries sh if csh doesn't exist).  If  only  one
           argument  is  given  to *_p_r_o_c_e_s_s, *_p_r_o_c_e_s_s_P _w_a_i_t_s
           _f_o_r _t_h_e _n_e_w _p_r_o_c_e_s_s _t_o _d_i_e _a_n_d _t_h_e_n  _r_e_t_u_r_n_s  _t_h_e
           _e_x_i_t  _c_o_d_e  _o_f  _t_h_e  _n_e_w _p_r_o_c_e_s_s.  _I_f _m_o_r_e _t_w_o _o_r
           _t_h_r_e_e _a_r_g_u_m_e_n_t_s _a_r_e _g_i_v_e_n,  *_p_r_o_c_e_s_s  _s_t_a_r_t_s  _t_h_e
           _p_r_o_c_e_s_s  _a_n_d _t_h_e_n _r_e_t_u_r_n_s _a _l_i_s_t _w_h_i_c_h, _d_e_p_e_n_d_i_n_g
           _o_n _t_h_e _v_a_l_u_e _o_f _g__r_e_a_d_p _a_n_d _g__w_r_i_t_e_p, _m_a_y _c_o_n_t_a_i_n
           _i/_o  _p_o_r_t_s _f_o_r _c_o_m_m_u_n_c_a_t_i_n_g _w_i_t_h _t_h_e _n_e_w _p_r_o_c_e_s_s.
           _I_f _g__w_r_i_t_e_p _i_s _n_o_n  _n_i_l,  _t_h_e_n  _a  _p_o_r_t  _w_i_l_l  _b_e
           _c_r_e_a_t_e_d  _w_h_i_c_h  _t_h_e  _l_i_s_p _p_r_o_g_r_a_m _c_a_n _u_s_e _t_o _s_e_n_d
           _c_h_a_r_a_c_t_e_r_s _t_o _t_h_e _n_e_w _p_r_o_c_e_s_s.  _I_f _g__r_e_a_d_p _i_s _n_o_n
           _n_i_l,  _t_h_e_n  _a _p_o_r_t _w_i_l_l _b_e _c_r_e_a_t_e_d _w_h_i_c_h _t_h_e _l_i_s_p
           _p_r_o_g_r_a_m _c_a_n _u_s_e _t_o _r_e_a_d _c_h_a_r_a_c_t_e_r_s _f_r_o_m  _t_h_e  _n_e_w
           _p_r_o_c_e_s_s.   _T_h_e  _v_a_l_u_e  _r_e_t_u_r_n_e_d  _b_y  *_p_r_o_c_e_s_s  _i_s
           (_r_e_a_d_p_o_r_t _w_r_i_t_e_p_o_r_t _p_i_d) _w_h_e_r_e _r_e_a_d_p_o_r_t _a_n_d  _w_r_i_-
           _t_e_p_o_r_t  _a_r_e  _e_i_t_h_e_r  _n_i_l  _o_r  _a _p_o_r_t _b_a_s_e_d _o_n _t_h_e
           _v_a_l_u_e _o_f _g__r_e_a_d_p _a_n_d _g__w_r_i_t_e_p.  _p_i_d _i_s  _t_h_e  _p_r_o_-
           _c_e_s_s  _i_d _o_f _t_h_e _n_e_w _p_r_o_c_e_s_s.  _S_i_n_c_e _i_t _i_s _h_a_r_d _t_o
           _r_e_m_e_m_b_e_r _t_h_e _o_r_d_e_r _o_f _g__r_e_a_d_p _a_n_d  _g__w_r_i_t_e_p,  _t_h_e
           _f_u_n_c_t_i_o_n_s *_p_r_o_c_e_s_s-_s_e_n_d _a_n_d *_p_r_o_c_e_s_s-_r_e_c_e_i_v_e _w_e_r_e
           _w_r_i_t_t_e_n _t_o _p_e_r_f_o_r_m _t_h_e _c_o_m_m_o_n _f_u_n_c_t_i_o_n_s.


                                    Printed: October 7, 1982







System Functions                                         6-7


(*process-receive '_s_t__c_o_m_m_a_n_d)

     RETURNS: a port which can be read.

     SIDE EFFECT: The command st_command  is  given  to  the
                  shell  and  it  is  started  runing in the
                  background.  The output of that command is
                  available   for   reading   via  the  port
                  returned.  The input of the  command  pro-
                  cess is set to /dev/null.

(*process-send 'st_command)

     RETURNS: a port which can be written to.

     SIDE EFFECT: The command st_command  is  given  to  the
                  shell  and  it  is  started  runing in the
                  background.  The lisp program can  provide
                  input  for that command by sending charac-
                  ters to the port returned  by  this  func-
                  tion.   The  output of the command process
                  is set to /dev/null.

(process s_pgrm [s_frompipe s_topipe])

     RETURNS: if the optional arguments are  not  present  a
              fixnum  which  is  the  exit  code when s_prgm
              dies.  If the optional arguments are  present,
              it returns a fixnum which is the process id of
              the child.

     NOTE: This command is obsolete.   New  programs  should
           use one of the *_p_r_o_c_e_s_s commands given above.

     SIDE EFFECT: If s_frompipe and s_topipe are given, they
                  are  bound  to ports which are pipes which
                  direct characters from FRANZ LISP  to  the
                  new process and to FRANZ LISP from the new
                  process  respectively.   _P_r_o_c_e_s_s  forks  a
                  process  named  s_prgm and waits for it to
                  die iff there are no pipe arguments given.











9

9                                    Printed: October 7, 1982







System Functions                                         6-8


(ptime)

     RETURNS: a list of  two  elements,  the  first  is  the
              amount of processor time used by the lisp sys-
              tem so far, the second is the amount  of  time
              used by the garbage collector so far.

     NOTE: the time is measured in 60_t_hs of a  second.   The
           first  number  includes  the  second number.  The
           amount of time used by garbage collection is  not
           recorded  until the first call to ptime.  This is
           done to prevent overhead when  the  user  is  not
           interested in garbage collection times.

(reset)

     SIDE EFFECT: the lisp runtime stack is cleared and  the
                  system  restarts  at the top level by exe-
                  cuting a (_f_u_n_c_a_l_l _t_o_p-_l_e_v_e_l _n_i_l).

(restorelisp 's_name)

     SIDE EFFECT: this  reads  in  file  s_name  (which  was
                  created  by  _s_a_v_e_l_i_s_p)  and  then  does  a
                  (_r_e_s_e_t).

     NOTE: This is only used on VMS systems  since  _d_u_m_p_l_i_s_p
           cannot be used.

(retbrk ['x_level])

     WHERE:   x_level is a small integer of either sign.

     SIDE EFFECT: The default error handler keeps  a  notion
                  of  the current level of the error caught.
                  If x_level is negative, control is  thrown
                  to  this default error handler whose level
                  is that many less than the present, or  to
                  _t_o_p-_l_e_v_e_l  if  there  aren't  enough.   If
                  x_level is non-negative, control is passed
                  to  the handler at that level.  If x_level
                  is not present, the value -1 is  taken  by
                  default.









9

9                                    Printed: October 7, 1982







System Functions                                         6-9


(*rset 'g_flag)

     RETURNS: g_flag

     SIDE EFFECT: If g_flag is non nil then the lisp  system
                  will   maintain  extra  information  about
                  calls to _e_v_a_l and  _f_u_n_c_a_l_l.   This  record
                  keeping slows down the evaluation but this
                  is required  for the  functions  _e_v_a_l_h_o_o_k,
                  _f_u_n_c_a_l_l_h_o_o_k,  and  _e_v_a_l_f_r_a_m_e  to  work. To
                  debug  compiled  lisp  code  the  transfer
                  tables       should      be      unlinked:
                  (_s_s_t_a_t_u_s _t_r_a_n_s_l_i_n_k _n_i_l)

(savelisp 's_name)

     RETURNS: t

     SIDE EFFECT: the state of the Lisp system is  saved  in
                  the  file  s_name.   It  can be read in by
                  _r_e_s_t_o_r_e_l_i_s_p.

     NOTE: This is only used on VMS systems  since  _d_u_m_p_l_i_s_p
           cannot be used.

(segment 's_type 'x_size)

     WHERE:   s_type is one of the data types given in 1.3

     RETURNS: a  segment  of  contiguous  lispvals  of  type
              s_type.

     NOTE: In reality, _s_e_g_m_e_n_t returns a new  data  cell  of
           type  s_type  and  allocates space for x_size - 1
           more s_type's beyond the one  returned.   _S_e_g_m_e_n_t
           always  allocates  new  space  and does so in 512
           byte chunks.  If you ask for 2  fixnums,  segment
           will  actually  allocate 128 of them thus wasting
           126 fixnums.  The  function  _s_m_a_l_l-_s_e_g_m_e_n_t  is  a
           smarter  space allocator and should be used when-
           ever possible.











9

9                                    Printed: October 7, 1982







System Functions                                        6-10


(shell)

     RETURNS: the exit code of the shell when it dies.

     SIDE EFFECT: this forks a new shell  and  returns  when
                  the shell dies.

(showstack)

     RETURNS: nil

     SIDE EFFECT: all  forms  currently  in  evaluation  are
                  printed,  beginning  with the most recent.
                  For compiled code the most that  showstack
                  will  show is the function name and it may
                  miss some functions.

(signal 'x_signum 's_name)

     RETURNS: nil if no previous call  to  signal  has  been
              made, or the previously installed s_name.

     SIDE EFFECT: this  declares  that  the  function  named
                  s_name   will  handle  the  signal  number
                  x_signum.  If s_name is nil, the signal is
                  ignored.  Presently only four UNIX signals
                  are caught, they and  their  numbers  are:
                  Interrupt(2),    Floating    exception(8),
                  Alarm(14), and Hang-up(1).

(sizeof 'g_arg)

     RETURNS: the number of  bytes  required  to  store  one
              object of type g_arg, encoded as a fixnum.

(small-segment 's_type 'x_cells)

     WHERE:   s_type is one of fixnum, flonum and value.

     RETURNS: a segment of  x_cells  data  objects  of  type
              s_type.

     SIDE EFFECT: This may  call  _s_e_g_m_e_n_t  to  allocate  new
                  space  or  it  may  be  able  to  fill the
                  request on a page already allocated.   The
                  value returned by _s_m_a_l_l-_s_e_g_m_e_n_t is usually
                  stored in the data  subpart  of  an  array
                  object.




9

9                                    Printed: October 7, 1982







System Functions                                        6-11


(sstatus g_type g_val)

     RETURNS: g_val

     SIDE EFFECT: If  g_type  is  not  one  of  the  special
                  sstatus  codes  described  in the next few
                  pages this simply sets g_val as the  value
                  of status type g_type in the system status
                  property list.

(sstatus appendmap g_val)

     RETURNS: g_val

     SIDE EFFECT: If g_val is non nil then when _f_a_s_l is told
                  to  create  a  load map, it will append to
                  the file name given in the  _f_a_s_l  command,
                  rather  than creating a new map file.  The
                  initial value is nil.

(sstatus automatic-reset g_val)

     RETURNS: g_val

     SIDE EFFECT: If g_val is non nil  then  when  an  error
                  occurs  which  no  one  wants to handle, a
                  _r_e_s_e_t will be done instead of  entering  a
                  primitive  internal  break loop.  The ini-
                  tial value is t.

(sstatus chainatom g_val)

     RETURNS: g_val

     SIDE EFFECT: If g_val is non nil and a _c_a_r or _c_d_r of  a
                  symbol  is done, then nil will be returned
                  instead of an error being signaled.   This
                  only affects the interpreter, not the com-
                  piler.  The initial value is nil.

(sstatus dumpcore g_val)

     RETURNS: g_val

     SIDE EFFECT: If g_val is nil,  FRANZ  LISP  tells  UNIX
                  that a segmentation violation or bus error
                  should cause a core dump.  If g_val is non
                  nil  then  FRANZ  LISP  will  catch  those
                  errors and print a  message  advising  the
                  user to reset.

     NOTE: The initial value for this flag is nil, and  only
           those  knowledgeable  of  the innards of the lisp


                                    Printed: October 7, 1982







System Functions                                        6-12


           system should ever set this flag non nil.

(sstatus dumpmode x_val)

     RETURNS: x_val

     SIDE EFFECT: All subsequent _d_u_m_p_l_i_s_p's will be done  in
                  mode  x_val.   x_val  may be either 413 or
                  410 (decimal).

     NOTE: the advantage of mode 413 is that the dumped Lisp
           can  be demand paged in when first started, which
           will make it start faster and disrupt other users
           less.  The initial value is 413.

(sstatus evalhook g_val)

     RETURNS: g_val

     SIDE EFFECT: When g_val is non nil,  this  enables  the
                  evalhook and  and funcallhook traps in the
                  evaluator.  See 14.4 for more details.

(sstatus feature g_val)

     RETURNS: g_val

     SIDE EFFECT: g_val is added  to  the  (_s_t_a_t_u_s _f_e_a_t_u_r_e_s)
                  list,

(sstatus gcstrings g_val)

     RETURNS: g_val

     SIDE EFFECT: if g_val is non nil and if string  garbage
                  collection  was enabled when the lisp sys-
                  tem was created, string space will be gar-
                  bage collected.

     NOTE: the default value for this is nil since  in  most
           applications  garbage  collecting  strings  is  a
           waste of time.










9

9                                    Printed: October 7, 1982







System Functions                                        6-13


(sstatus ignoreeof g_val)

     RETURNS: g_val

     SIDE EFFECT: If g_val is non nil then if a end of  file
                  (CNTL D on UNIX) is typed to the top level
                  interpreter it will be ignored rather then
                  cause the lisp system to exit.  If the the
                  standard input is  a  file  or  pipe  then
                  this  has  no  effect,  a  EOF will always
                  cause lisp to exit.  The initial value  is
                  nil.

(sstatus nofeature g_val)

     RETURNS: g_val

     SIDE EFFECT: g_val is removed from the status  features
                  list if it was present.

(sstatus translink g_val)

     RETURNS: g_val

     SIDE EFFECT: If g_val is  nil then all transfer  tables
                  are  cleared and further calls through the
                  transfer table will  not  cause  the  fast
                  links  to be set up.  If g_val is the sym-
                  bol _o_n then all  possible  transfer  table
                  entries  will  be linked and the flag will
                  be set to cause fast links to  be  set  up
                  dynamically.   Otherwise  all that is done
                  is to set the flag to cause fast links  to
                  be  set up dynamically.  The initial value
                  is nil.

     NOTE: For a discussion of transfer tables, see the Sec-
           tion on the compiler.

(sstatus uctolc g_val)

     RETURNS: g_val

     SIDE EFFECT: If g_val is not  nil  then  all  unescaped
                  capital  letters  in  symbols  read by the
                  reader will be converted to lower case.

     NOTE: This allows FRANZ LISP to be compatible with sin-
           gle  case  lisp  systems (e.g. Maclisp, Interlisp
           and UCILisp).


9

9                                    Printed: October 7, 1982







System Functions                                        6-14


(status g_code)

     RETURNS: the value  associated  with  the  status  code
              g_code  if  g_code  is  not one of the special
              cases given below

(status ctime)

     RETURNS: a symbol whose print name is the current  time
              and date.

     EXAMPLE: (_s_t_a_t_u_s _c_t_i_m_e) ==> |Sun Jun 29 16:51:26 1980|

(status feature g_val)

     RETURNS: t iff g_val is in the status features list.

(status features)

     RETURNS: the value of the features  code,  which  is  a
              list  of  features  which  are present in this
              system.    You   add   to   this   list   with
              (_s_s_t_a_t_u_s _f_e_a_t_u_r_e '_g__v_a_l)  and  test if feature
              g_feat         is         present         with
              (_s_t_a_t_u_s _f_e_a_t_u_r_e '_g__f_e_a_t).

(status isatty)

     RETURNS: t iff the standard input is a terminal.

(status localtime)

     RETURNS: a list of  fixnums  representing  the  current
              time.

     EXAMPLE: (_s_t_a_t_u_s _l_o_c_a_l_t_i_m_e) ==>  (3 51 13 31 6 81 5 211
              1)
              means 3_r_d second, 51_s_t minute,  13_t_h  hour  (1
              p.m), 31_s_t day, month 6 (0 = January), year 81
              (0 = 1900), day of the  week  5  (0 = Sunday),
              211_t_h  day  of  the  year and daylight savings
              time is in effect.










9

9                                    Printed: October 7, 1982







System Functions                                        6-15


(status syntax s_char)

     NOTE: This  function  should  not  be  used.   See  the
           description  of  _g_e_t_s_y_n_t_a_x  (in  Chapter 7) for a
           replacement.

(status undeffunc)

     RETURNS: a list of all functions which  transfer  table
              entries  point to but which are not defined at
              this point.

     NOTE: Some of the undefined functions listed  could  be
           arrays which have yet to be created.

(status version)

     RETURNS: a string which is  the  current  lisp  version
              name.

     EXAMPLE: (_s_t_a_t_u_s _v_e_r_s_i_o_n) ==> "Franz Lisp, Opus 33b"

(syscall 'x_index ['xst_arg1 ...])

     RETURNS: the result of issuing  the  UNIX  system  call
              number x_index with arguments xst_arg_i.

     NOTE: The UNIX system calls are described in section  2
           of the UNIX manual. If xst_arg_i is a fixnum, then
           its value is passed as an argument, if  it  is  a
           symbol then its pname is passed and finally if it
           is a string then the string itself is  passed  as
           an argument.  Some useful syscalls are:
           (_s_y_s_c_a_l_l _2_0) returns process id.
           (_s_y_s_c_a_l_l _1_3) returns the number of seconds  since
           Jan 1, 1970.
           (_s_y_s_c_a_l_l _1_0 '_f_o_o) will unlink (delete)  the  file
           foo.

(top-level)

     RETURNS: nothing (it never returns)

     NOTE: This function is  the  top-level  read-eval-print
           loop.   It  never  returns  any  value.  Its main
           utility is that if you  redefine  it,  and  do  a
           (reset)  then  the  redefined (top-level) is then
           invoked.




9

9                                    Printed: October 7, 1982







System Functions                                        6-16


(wait)

     RETURNS: a dotted pair (_p_r_o_c_e_s_s_i_d .  _s_t_a_t_u_s)  when  the
              next child  process  dies.
















































9

9                                    Printed: October 7, 1982



