






                         CHAPTER  4


                         Special Functions




(and [g_arg1 ...])

     RETURNS: the value of the last argument  if  all  argu-
              ments  evaluate  to a non nil value, otherwise
              _a_n_d returns nil.  It returns t if there are no
              arguments.

     NOTE: the arguments are evaluated  left  to  right  and
           evaluation  will cease with the first nil encoun-
           tered

(apply 'u_func 'l_args)

     RETURNS: the result of applying function u_func to  the
              arguments in the list l_args.

     NOTE: If u_func is a lambda, then  the  (_l_e_n_g_t_h _l__a_r_g_s)
           should  equal the number of formal parameters for
           the u_func.  If u_func is  a  nlambda  or  macro,
           then l_args is bound to the single formal parame-
           ter.























9

9   Special Functions                                     4-1







   Special Functions                                     4-2



    ____________________________________________________

    ; _a_d_d_1 is a lambda of 1 argument
    -> (_a_p_p_l_y '_a_d_d_1 '(_3))
    4

    ; we will define _p_l_u_s_1 as a macro which will be equivalent to _a_d_d_1
    -> (_d_e_f _p_l_u_s_1 (_m_a_c_r_o (_a_r_g) (_l_i_s_t '_a_d_d_1 (_c_a_d_r _a_r_g))))
    plus1
    -> (_p_l_u_s_1 _3)
    4

    ; now if we _a_p_p_l_y a macro we obtain the form it changes to.
    -> (_a_p_p_l_y '_p_l_u_s_1 '(_p_l_u_s_1 _3))
    (add1 3)

    ; if we _f_u_n_c_a_l_l a macro however, the result of the macro is _e_v_a_led
    ; before it is returned.
    -> (_f_u_n_c_a_l_l '_p_l_u_s_1 '(_p_l_u_s_1 _3))
    4

    ; for this particular macro, the _c_a_r of the _a_r_g is not checked
    ; so that this too will work
    -> (_a_p_p_l_y '_p_l_u_s_1 '(_f_o_o _3))
    (add1 3)

    ____________________________________________________




(arg ['x_numb])

     RETURNS: if x_numb  is  specified  then  the  x_numb'_t_h
              argument  to  the enclosing lexpr If x_numb is
              not specified then this returns the number  of
              arguments to the enclosing lexpr.

     NOTE: it is an error to the interpreter  if  x_numb  is
           given and out of range.











9

9                                 Printed: September 21, 1982







   Special Functions                                     4-3


(break [g_message ['g_pred]])

     WHERE:   if g_message is not given it is assumed to  be
              the null string, and if g_pred is not given it
              is assumed to be t.

     RETURNS: the value of (*_b_r_e_a_k '_g__p_r_e_d '_g__m_e_s_s_a_g_e)

(*break 'g_pred 'g_message)

     RETURNS: nil immediately if g_pred  is  nil,  else  the
              value  of  the next (return 'value) expression
              typed in at top level.

     SIDE EFFECT: If the predicate, g_pred, evaluates to non
                  nil,  the lisp system stops and prints out
                  `Break ' followed by  g_message.  It  then
                  enters  a  break  loop which allows one to
                  interactively debug a  program.   To  con-
                  tinue  execution  from a break you can use
                  the _r_e_t_u_r_n  function.  to  return  to  top
                  level  or another break level, you can use
                  _r_e_t_b_r_k or _r_e_s_e_t.

(catch g_exp [ls_tag])

     WHERE:   if ls_tag is not given, it is  assumed  to  be
              nil.

     RETURNS: the result of (*_c_a_t_c_h '_l_s__t_a_g _g__e_x_p)

     NOTE: catch is defined as a macro.

(*catch 'ls_tag g_exp)

     WHERE:   ls_tag is either a symbol or a  list  of  sym-
              bols.

     RETURNS: the result of evaluating g_exp  or  the  value
              thrown during the evaluation of g_exp.

     SIDE EFFECT: this first sets up a `catch frame' on  the
                  lisp  runtime  stack.   Then  it begins to
                  evaluate g_exp.  If g_exp  evaluates  nor-
                  mally,  its  value  is returned.  If, how-
                  ever, a value is thrown during the evalua-
                  tion of g_exp then this *catch will return
                  with that value if one of these  cases  is
                  true:

     (1)  the tag thrown to is ls_tag

9

9                                 Printed: September 21, 1982







   Special Functions                                     4-4


     (2)  ls_tag is a list and the tag thrown to is a member
          of this list

     (3)  ls_tag is nil.

     NOTE: Errors are  implemented  as  a  special  kind  of
           throw.   A  catch  with  no tag will not catch an
           error but a catch whose tag  is  the  error  type
           will  catch  that  type of error.  See Chapter 10
           for more information.

(comment [g_arg ...])

     RETURNS: the symbol comment.

     NOTE: This does absolutely nothing.

(cond [l_clause1 ...])

     RETURNS: the last value evaluated in the  first  clause
              satisfied.   If  no clauses are satisfied then
              nil is returned.

     NOTE: This is  the  basic  conditional  `statement'  in
           lisp.   The  clauses  are  processed from left to
           right.   The  first  element  of  a   clause   is
           evaluated.   If  it  evaluated to a non nil value
           then that clause is satisfied and  all  following
           elements  of that clause are evaluated.  The last
           value computed is returned as the  value  of  the
           cond.  If there is just one element in the clause
           then its value is returned.  If the first element
           of a clause evaluates to nil, then the other ele-
           ments of that clause are not  evaluated  and  the
           system moves to the next clause.

(cvttointlisp)

     SIDE EFFECT: The reader is modified to conform with the
                  Interlisp syntax.  The character % is made
                  the escape character and special  meanings
                  for  comma,  backquote  and  backslash are
                  removed. Also the reader is told  to  con-
                  vert upper case to lower case.








9

9                                 Printed: September 21, 1982







   Special Functions                                     4-5


(cvttomaclisp)

     SIDE EFFECT: The reader is  modified  to  conform  with
                  Maclisp  syntax.   The character / is made
                  the escape character and the special mean-
                  ings for backslash, left and right bracket
                  are removed.  Also the reader is  told  to
                  convert upper case to lower case.

(cvttoucilisp)

     SIDE EFFECT: The reader is  modified  to  conform  with
                  Maclisp  syntax.   The character / is made
                  the escape character, tilde  is  made  the
                  comment character, exclamation point takes
                  on the unquote function normally  held  by
                  comma,  and  backslash,  comma,  semicolon
                  become normal characters.  Also the reader
                  is  told  to  convert  upper case to lower
                  case.

(debug s_msg)

     SIDE EFFECT: Enter  the  Fixit  package  described   in
                  Chapter  15.   This  package allows you to
                  examine the evaluation  stack  in  detail.
                  To  leave the Fixit package type 'ok'.

(debugging 'g_arg)

     SIDE EFFECT: This unlinks the transfer tables,  does  a
                  (*_r_s_e_t _t) to turn on evaluation monitoring
                  and sets the all-error catcher (ER%all) to
                  be _d_e_b_u_g-_e_r_r-_h_a_n_d_l_e_r.

(declare [g_arg ...])

     RETURNS: nil

     NOTE: this is a no-op to the evaluator.  It has special
           meaning to the compiler (see Chapter 12).











9

9                                 Printed: September 21, 1982







   Special Functions                                     4-6


(def s_name (s_type l_argl g_exp1 ...))

     WHERE:   s_type is one of  lambda,  nlambda,  macro  or
              lexpr.

     RETURNS: s_name

     SIDE EFFECT: This defines the function  s_name  to  the
                  lisp  system.   If  s_type  is  nlambda or
                  macro then the argument list  l_argl  must
                  contain exactly one non-nil symbol.

(defmacro s_name l_arg g_exp1 ...)

     RETURNS: s_name

     SIDE EFFECT: This defines the  macro  s_name.  _d_e_f_m_a_c_r_o
                  makes  it  easy  to  write macros since it
                  makes the syntax just like _d_e_f_u_n.  Further
                  information on _d_e_f_m_a_c_r_o is in 8.3.2.

(defun s_name [s_mtype] ls_argl g_exp1 ... )

     WHERE:   s_mtype is one of fexpr, expr, args or macro.

     RETURNS: s_name

     SIDE EFFECT: This defines the function s_name.

     NOTE: this exists for Maclisp compatibility, it is just
           a  macro  which changes the defun form to the def
           form.   An  s_mtype  of  fexpr  is  converted  to
           nlambda  and of expr to lambda. Macro remains the
           same.  If ls_arg1 is a non-nil symbol,  then  the
           type  is  assumed  to be lexpr and ls_arg1 is the
           symbol which is bound to the number of args  when
           the function is entered.


    ____________________________________________________

    ; _d_e_f and _d_e_f_u_n here are used to define identical functions
    ; you can decide for yourself which is easier to use.
    -> (_d_e_f _a_p_p_e_n_d_1 (_l_a_m_b_d_a (_l_i_s _e_x_t_r_a) (_a_p_p_e_n_d _l_i_s (_l_i_s_t _e_x_t_r_a))))
    append1

    -> (_d_e_f_u_n _a_p_p_e_n_d_1 (_l_i_s _e_x_t_r_a) (_a_p_p_e_n_d _l_i_s (_l_i_s_t _e_x_t_r_a)))
    append1
    ____________________________________________________



9

9                                 Printed: September 21, 1982







   Special Functions                                     4-7


(do l_vrbs l_test g_exp1 ...)

     RETURNS: the last form in the cdr of l_test  evaluated,
              or  a  value  explicitly  given  by  a  return
              evaluated within the do body.

     NOTE: This is the basic iteration form for FRANZ  LISP.
           l_vrbs  is a list of zero or more var-init-repeat
           forms.  A var-init-repeat form looks like:
                (s_name [g_init [g_repeat]])
           There  are  three  cases  depending  on  what  is
           present  in the form.  If just s_name is present,
           this means that when the do is entered, s_name is
           lambda-bound  to nil and is never modified by the
           system (though the program is certainly  free  to
           modify    its    value).     If   the   form   is
           (s_name 'g_init) then the only difference is that
           s_name  is  lambda-bound  to  the value of g_init
           instead of nil.  If g_repeat is also present then
           s_name is lambda-bound to g_init when the loop is
           entered and after each pass through the  do  body
           s_name is  bound to the value of g_repeat.
           l_test is either nil or has the form  of  a  cond
           clause.   If  it  is nil then the do body will be
           evaluated only once and the do will  return  nil.
           Otherwise,  before  the  do body is evaluated the
           car of l_test is evaluated and if the  result  is
           non nil this signals an end to the looping.  Then
           the rest of the forms in l_test are evaluated and
           the  value  of  the  last  one is returned as the
           value of the do.  If the cdr of  l_test  is  nil,
           then  nil is returned -- thus this is not exactly
           like a cond clause.
           g_exp1 and those forms  which  follow  constitute
           the  do  body.  A do body is like a prog body and
           thus may have labels and one may  use  the  func-
           tions go and return.
           The sequence of evaluations is this:

     (1)  the init forms are evaluated  left  to  right  and
          stored in temporary locations.

     (2)  Simultaneously all do variables are  lambda  bound
          to the value of their init forms or nil.

     (3)  If l_test is non nil then the car is evaluated and
          if  it  is non nil the rest of the forms in l_test
          are evaluated and the last value  is  returned  as
          the value of the do.

     (4)  The forms in the do body  are  evaluated  left  to
          right.
9

9                                 Printed: September 21, 1982







   Special Functions                                     4-8


     (5)  If l_test is nil the do function returns with  the
          value nil.

     (6)  The repeat forms are evaluated and saved  in  tem-
          porary locations.

     (7)  The variables with repeat forms are simultaneously
          bound to the values of those forms.

     (8)  Go to step 3.

     NOTE: there is an alternate form of  do  which  can  be
           used  when  there is only one do variable.  It is
           described next.


    ____________________________________________________

    ; this is  a simple function which numbers the elements of a list.
    ; It uses a _d_o function with two local variables.
    -> (_d_e_f_u_n _p_r_i_n_t_e_m (_l_i_s)
                 (_d_o ((_x_x _l_i_s (_c_d_r _x_x))
                      (_i _1 (_1+ _i)))
                     ((_n_u_l_l _x_x) (_p_a_t_o_m "_a_l_l _d_o_n_e") (_t_e_r_p_r))
                     (_p_r_i_n_t _i)
                     (_p_a_t_o_m ": ")
                     (_p_r_i_n_t (_c_a_r _x_x))
                     (_t_e_r_p_r)))
    printem
    -> (_p_r_i_n_t_e_m '(_a _b _c _d))
    1: a
    2: b
    3: c
    4: d
    all done
    nil
    ->
    ____________________________________________________














9

9                                 Printed: September 21, 1982







   Special Functions                                     4-9


(do s_name g_init g_repeat g_test g_exp1 ...)

     NOTE: this is another, less general,  form of  do.   It
           is evaluated by:

     (1)  evaluating g_init

     (2)  lambda binding s_name to value of g_init

     (3)  g_test is evaluated and if it is not  nil  the  do
          function returns with nil.

     (4)  the do body is evaluated beginning at g_exp1.

     (5)  the repeat form is evaluated and stored in s_name.

     (6)  go to step 3.

(err ['s_value [nil]])

     RETURNS: nothing (it never returns).

     SIDE EFFECT: This causes an error and if this error  is
                  caught  by an _e_r_r_s_e_t then that _e_r_r_s_e_t will
                  return s_value instead  of  nil.   If  the
                  second  arg  is given, then it must be nil
                  (MAClisp compatibility).

(error ['s_message1 ['s_message2]])

     RETURNS: nothing (it never returns).

     SIDE EFFECT: s_message1 and s_message2 are  _p_a_t_o_med  if
                  they  are  given  and  then  _e_r_r is called
                  which causes an error.

(errset g_expr [s_flag])

     RETURNS: a list of one  element,  which  is  the  value
              resulting from evaluating g_expr.  If an error
              occurs during the evaluation of  g_expr,  then
              the locus of control will return to the _e_r_r_s_e_t
              which will then return nil (unless  the  error
              was caused by a call to _e_r_r).

     SIDE EFFECT: S_flag  is  evaluated  before  g_expr   is
                  evaluated. If s_flag is not given, then it
                  is assumed to be t.  If  an  error  occurs
                  during   the  evaluation  of  g_expr,  and
                  s_flag evaluated to a non nil value,  then
                  the  error  message  associated  with  the
                  error is printed before control returns to
                  the errset.


                                 Printed: September 21, 1982







   Special Functions                                    4-10


(eval 'g_val ['x_bind-pointer])

     RETURNS: the result of evaluating g_val.

     NOTE: The evaluator evaluates g_val in this way:
           If g_val is a symbol, then the evaluator  returns
           its  value.   If  g_val had never been assigned a
           value, then this  causes  an  'Unbound  Variable'
           error.   If  x_bind-pointer  is  given,  then the
           variable  is  evaluated  with  respect  to   that
           pointer  (see  _e_v_a_l_f_r_a_m_e  for  details  on  bind-
           pointers).  If g_val is of type value,  then  its
           value  is  returned.   If  g_val is a list object
           then g_val is either a  function  call  or  array
           reference.   Let  g_car  be  the first element of
           g_val.  We continually evaluate  g_car  until  we
           end  up  with  a  symbol  with a non nil function
           binding or a non-symbol.  Call  what  we  end  up
           with: g_func.  g_func must be one of three types:
           list, binary or array.  If it is a list then  the
           first  element  of  the list, which we shall call
           g_functype, must be either lambda, nlambda, macro
           or  lexpr.   If g_func is a binary, then its dis-
           cipline,  which  we  shall  call  g_functype,  is
           either  lambda,  nlambda, macro or a string "sub-
           routine",   "function",   "integer-function"   or
           "real-function".  If g_func is an array then this
           form is evaluated specially, see 9 on arrays.  If
           g_func  is a list or binary, then g_functype will
           determine how the arguments to this function, the
           cdr  of g_val, are processed.  If g_functype is a
           string, then this is a foreign function call (see
           8.4  for  more details).  If g_functype is lambda
           or lexpr, the arguments are  evaluated  (by  cal-
           ling   _e_v_a_l   recursively)   and   stacked.    If
           g_functype is nlambda then the argument  list  is
           stacked.   If g_functype is macro then the entire
           form, g_val is stacked.  Next  the  formal  vari-
           ables are lambda bound.  The formal variables are
           the cadr of g_func - if  g_functype  is  nlambda,
           lexpr  or  macro, there should only be one formal
           variable.  The values on  the  stack  are  lambda
           bound  to the formal variables except in the case
           of a lexpr, where the number of actual  arguments
           is bound to the formal variable.  After the bind-
           ing is done, the function is invoked,  either  by
           jumping  to  the  entry  point  in  the case of a
           binary or by evaluating the list of forms  begin-
           ning at cddr g_func.  The result of this function
           invocation is returned as the value of  the  call
           to eval.

9

9                                 Printed: September 21, 1982







   Special Functions                                    4-11


(evalframe 'x_pdlpointer)

     RETURNS: an evalframe  descriptor  for  the  evaluation
              frame    just    before    x_pdlpointer.    If
              x_pdlpointer is nil, it returns the evaluation
              frame  of  the  frame  just before the current
              call to _e_v_a_l_f_r_a_m_e.

     NOTE: An evalframe descriptor describes a call to _e_v_a_l,
           _a_p_p_l_y or _f_u_n_c_a_l_l.  The form of the descriptor is
           (_t_y_p_e  _p_d_l-_p_o_i_n_t_e_r  _e_x_p_r_e_s_s_i_o_n  _b_i_n_d-_p_o_i_n_t_e_r  _n_p-
           _i_n_d_e_x _l_b_o_t-_i_n_d_e_x)
           where type is `eval' if this describes a call  to
           _e_v_a_l  or  `apply'  if  this is a call to _a_p_p_l_y or
           _f_u_n_c_a_l_l.    pdl-pointer   is   a   number   which
           describes this context. It can be passed to _e_v_a_l_-
           _f_r_a_m_e to obtain the next descriptor  and  can  be
           passed  to  _f_r_e_t_u_r_n  to  cause a return from this
           context.  bind-pointer is the  size  of  variable
           binding  stack  when  this  evaluation began. The
           bind-pointer can be given as a second argument to
           _e_v_a_l  to  order to evaluate variables in the same
           context as this  evaluation. If  type  is  `eval'
           then  expression  will  have  the form (_f_u_n_c_t_i_o_n-
           _n_a_m_e _a_r_g_1 ...).  If type is `apply' then  expres-
           sion    will    have    the    form    (_f_u_n_c_t_i_o_n-
           _n_a_m_e (_a_r_g_1 ...)).  np-index  and  lbot-index  are
           pointers  into  the argument stack (also known as
           the _n_a_m_e_s_t_a_c_k array) at the time of call.   lbot-
           index  points  to  the  first  argument, np-index
           points one beyond the last argument.
           In order for there to be enough  information  for
           _e_v_a_l_f_r_a_m_e to return, you must call (*_r_s_e_t _t).

     EXAMPLE: (_p_r_o_g_n (_e_v_a_l_f_r_a_m_e _n_i_l))
              returns  (_e_v_a_l  _2_1_4_7_4_7_8_6_0_0  (_p_r_o_g_n  (_e_v_a_l_f_r_a_m_e
              _n_i_l)) _1 _8 _7)

(evalhook 'g_form 'su_evalfunc ['su_funcallfunc])

     RETURNS: the result of evaluating g_form  after  lambda
              binding  `evalhook'  to su_evalfunc and, if it
              is  given,  lambda  binding  `funcallhook'  to
              su_funcallhook.

     NOTE: As explained in 14.4, the function _e_v_a_l may  pass
           the  job  of  evaluating  a form to a user `hook'
           function when  various  switches  are  set.   The
           hook  function  normally  prints  the  form to be
           evaluated on the terminal and then  evaluates  it
           by  calling  _e_v_a_l_h_o_o_k.   _E_v_a_l_h_o_o_k does the lambda
           binding mentioned above and then  calls  _e_v_a_l  to
           evaluate  the  form  after  setting  an  internal


                                 Printed: September 21, 1982







   Special Functions                                    4-12


           switch to tell _e_v_a_l not to call the  user's  hook
           function  just  this  one  time.  This allows the
           evaluation process to advance one  step  and  yet
           insure  that  further  calls  to  _e_v_a_l will cause
           traps to the hook function (if su_evalfunc is non
           nil).
           In order for  _e_v_a_l_h_o_o_k  to  work,  (*_r_s_e_t _t)  and
           (_s_s_t_a_t_u_s _e_v_a_l_h_o_o_k _t)  must  have been done previ-
           ously.

(eval-when l_times g_exp1 ... g_expn)

     WHERE:   l_times is a list containing  any  combination
              of compile, eval and load.

     RETURNS: nil if the  symbol  eval  is  not   member  of
              l_times, else returns the value of g_expn.

     SIDE EFFECT: If eval is a member of l_times,  then  the
                  forms g_exp_i are evaluated.

     NOTE: this is used mainly to control when the  compiler
           evaluates forms.

(exec s_arg1 ...)

     RETURNS: the result of forking and executing  the  com-
              mand   named   by   concatenating  the  s_arg_i
              together with spaces in between.

(exece 's_fname ['l_args ['l_envir]])

     RETURNS: the error code  from  the  system  if  it  was
              unable  to  execute  the  command s_fname with
              arguments l_args and with the environment  set
              up  as specified in l_envir.  If this function
              is successful, it will not return, instead the
              lisp  system  will be overlaid by the new com-
              mand.

(freturn 'x_pdl-pointer 'g_retval)

     RETURNS: g_retval from  the  context  given  by  x_pdl-
              pointer.

     NOTE: A  pdl-pointer  denotes  a   certain   expression
           currently  being evaluated. The pdl-pointer for a
           given expression can be obtained from _e_v_a_l_f_r_a_m_e.




9

9                                 Printed: September 21, 1982







   Special Functions                                    4-13


(frexp 'f_arg)

     RETURNS: a dotted  pair  (_e_x_p_o_n_e_n_t  .  _m_a_n_t_i_s_s_a)  which
              represents the given flonum

     NOTE: The exponent will be a fixnum, the mantissa a  56
           bit bignum.  If you think of the the binary point
           occurring right  after  the  high  order  bit  of
           mantissa, then f_arg = 2[exponent] * mantissa.

(funcall 'u_func ['g_arg1 ...])

     RETURNS: the value of applying function u_func  to  the
              arguments  g_arg_i  and  then  evaluating  that
              result if u_func is a macro.

     NOTE: If u_func is a macro or nlambda then there should
           be only one g_arg.  _f_u_n_c_a_l_l is the function which
           the evaluator uses to evaluate lists.  If _f_o_o  is
           a    lambda    or    lexpr    or    array,   then
           (_f_u_n_c_a_l_l '_f_o_o '_a '_b '_c)    is    equivalent    to
           (_f_o_o '_a '_b '_c).    If   _f_o_o  is  a  nlambda  then
           (_f_u_n_c_a_l_l '_f_o_o '(_a _b _c)) is equivalent to (_f_o_o _a _b
           _c).    Finally,   if   _f_o_o   is   a   macro  then
           (_f_u_n_c_a_l_l '_f_o_o '(_f_o_o _a _b _c))  is   equivalent   to
           (_f_o_o _a _b _c).

(funcallhook 'l_form 'su_funcallfunc ['su_evalfunc])

     RETURNS: the result of _f_u_n_c_a_l_ling the  (_c_a_r _l__f_o_r_m)  on
              the   already   evaluated   arguments  in  the
              (_c_d_r _l__f_o_r_m)  after   lambda   binding   `fun-
              callhook'  to  su_funcallfunc  and,  if  it is
              given,   lambda    binding    `evalhook'    to
              su_evalhook.

     NOTE: This function is designed to continue the evalua-
           tion  process  with  as  little  work as possible
           after a funcallhook trap has occurred. It is  for
           this  reason  that the form of l_form is unortho-
           dox: its _c_a_r is the name of the function to  call
           and  its  _c_d_r  are  a  list of arguments to stack
           (without evaluating  again)  before  calling  the
           given function.  After stacking the arguments but
           before calling _f_u_n_c_a_l_l an internal switch is  set
           to  prevent  _f_u_n_c_a_l_l from passing the job of fun-
           calling to su_funcallfunc.  If _f_u_n_c_a_l_l is  called
           recursively   in   funcalling   l_form   and   if
           su_funcallfunc is non nil, then the arguments  to
           _f_u_n_c_a_l_l  will actually be given to su_funcallfunc
           (a lexpr) to be  funcalled.
           In order for  _e_v_a_l_h_o_o_k  to  work,  (*_r_s_e_t _t)  and
           (_s_s_t_a_t_u_s _e_v_a_l_h_o_o_k _t)    must   have   been   done


                                 Printed: September 21, 1982







   Special Functions                                    4-14


           previously.   A  more  detailed  description   of
           _e_v_a_l_h_o_o_k and _f_u_n_c_a_l_l_h_o_o_k is given in Chapter 14.

(function u_func)

     RETURNS: the function binding of u_func  if  it  is  an
              symbol   with  a  function  binding  otherwise
              u_func is returned.

(getdisc 'y_func)

     RETURNS: the discipline of the machine  coded  function
              (either lambda, nlambda or macro).

(go g_labexp)

     WHERE:   g_labexp is either a symbol or an expression.

     SIDE EFFECT: If g_labexp is an expression, that expres-
                  sion  is  evaluated and should result in a
                  symbol.  The locus  of  control  moves  to
                  just  following the symbol g_labexp in the
                  current prog or do body.

     NOTE: this is only valid in the context of a prog or do
           body.   The  interpreter  and compiler will allow
           non-local _g_o's although the compiler won't  allow
           a _g_o to leave a function body.  The compiler will
           not allow g_labexp to be an expression.

(I-throw-err 'l_token)

     WHERE:   l_token is the _c_d_r of the value returned  from
              a *_c_a_t_c_h with the tag ER%unwind-protect.

     RETURNS: nothing (never returns in the current context)

     SIDE EFFECT: The error or throw denoted by  l_token  is
                  continued.

     NOTE: This function is used to implement _u_n_w_i_n_d-_p_r_o_t_e_c_t
           which allows the processing of a transfer of con-
           trol though a certain context to be  interrupted,
           a  user  function  to  be  executed  and than the
           transfer of control to  continue.   The  form  of
           l_token is either
           (_t _t_a_g _v_a_l_u_e) for a throw or
           (_n_i_l _t_y_p_e _m_e_s_s_a_g_e _v_a_l_r_e_t  _c_o_n_t_u_a_b  _u_n_i_q_u_e_i_d  [_a_r_g
           ...]) for an error.
           This function is not to be used for  implementing
           throws  or errors and is only documented here for
           completeness.
9

9                                 Printed: September 21, 1982







   Special Functions                                    4-15


(let l_args g_exp1 ... g_exprn)

     RETURNS: the result of evaluating  g_exprn  within  the
              bindings given by l_args.

     NOTE: l_args is either nil (in which case _l_e_t  is  just
           like  _p_r_o_g_n)  or it is a list of binding objects.
           A binding object is a  list  (_s_y_m_b_o_l _e_x_p_r_e_s_s_i_o_n).
           When  a _l_e_t is entered all of the expressions are
           evaluated and then simultaneously lambda bound to
           the  corresponding  symbols.   In  effect,  a _l_e_t
           expression  is  just  like  a  lambda  expression
           except  the  symbols and their initial values are
           next to each other  which  makes  the  expression
           easier  to  understand.   There  are  some  added
           features to the _l_e_t expression: A binding  object
           can  just  be a symbol, in which case the expres-
           sion corresponding to that symbol is `nil'.  If a
           binding object is a list and the first element of
           that list is another  list,  then  that  list  is
           assumed  to be a binding template and _l_e_t will do
           a _d_e_s_e_t_q on it.

(let* l_args g_exp1 ... g_expn)

     RETURNS: the result of evaluating  g_exprn  within  the
              bindings given by l_args.

     NOTE: This is identical to _l_e_t except  the  expressions
           in  the  binding  list  l_args  are evaluated and
           bound sequentially instead of in parallel.

(listify 'x_count)

     RETURNS: a list of x_count  of  the  arguments  to  the
              current function (which must be a lexpr).

     NOTE: normally  arguments   1   through   x_count   are
           returned.  If x_count is negative then  a list of
           last abs(x_count) arguments are returned.

(map 'u_func 'l_arg1 ...)

     RETURNS: l_arg1

     NOTE: The function u_func is applied to successive sub-
           lists  of  the  l_arg_i.  All sublists should have
           the same length.




9

9                                 Printed: September 21, 1982







   Special Functions                                    4-16


(mapc 'u_func 'l_arg1 ...)

     RETURNS: l_arg1.

     NOTE: The function u_func is applied to successive ele-
           ments  of  the  argument lists.  All of the lists
           should have the same length.

(mapcan 'u_func 'l_arg1 ...)

     RETURNS: nconc applied to the results of the functional
              evaluations.

     NOTE: The function u_func is applied to successive ele-
           ments of the argument lists.  All sublists should
           have the same length.

(mapcar 'u_func 'l_arg1 ...)

     RETURNS: a list of the values returned from  the  func-
              tional application.

     NOTE: the function u_func is applied to successive ele-
           ments of the argument lists.  All sublists should
           have the same length.

(mapcon 'u_func 'l_arg1 ...)

     RETURNS: nconc applied to the results of the functional
              evaluation.

     NOTE: the function u_func is applied to successive sub-
           lists of the argument lists.  All sublists should
           have the same length.

(maplist 'u_func 'l_arg1 ...)

     RETURNS: a  list  of  the  results  of  the  functional
              evaluations.

     NOTE: the function u_func is applied to successive sub-
           lists  of  the  arguments  lists.   All  sublists
           should have the same length.









9

9                                 Printed: September 21, 1982







   Special Functions                                    4-17


(mfunction t_entry 's_disc)

     RETURNS: a lisp  object  of  type  binary  composed  of
              t_entry and s_disc.

     NOTE: t_entry is a pointer to the machine  code  for  a
           function,  and  s_disc  is  the  discipline (e.g.
           lambda).

(oblist)

     RETURNS: a list of all symbols on the oblist.

(or [g_arg1 ... ])

     RETURNS: the value of the first non  nil  argument   or
              nil if all arguments evaluate to nil.

     NOTE: Evaluation proceeds left to right  and  stops  as
           soon  as  one of the arguments evaluates to a non
           nil value.

(prog l_vrbls g_exp1 ...)

     RETURNS: the value explicitly given in a return form or
              else  nil if no return is done by the time the
              last g_exp_i is evaluated.

     NOTE: the local variables are lambda bound to nil  then
           the g_exp are evaluated from left to right.  This
           is a prog body (obviously) and  this  means  than
           any  symbols seen are not evaluated, instead they
           are treated as  labels.   This  also  means  that
           returns and go's are allowed.

(prog1 'g_exp1 ['g_exp2 ...])

     RETURNS: g_exp1

(prog2 'g_exp1 'g_exp2 ['g_exp3 ...])

     RETURNS: g_exp2

     NOTE: the forms are evaluated from left  to  right  and
           the value of g_exp2 is returned.







9

9                                 Printed: September 21, 1982







   Special Functions                                    4-18


(progn 'g_exp1 ['g_exp2 ...])

     RETURNS: the last g_exp_i.

(progv 'l_locv 'l_initv g_exp1 ...)

     WHERE:   l_locv is a list of symbols and l_initv  is  a
              list of expressions.

     RETURNS: the value of the last g_exp_i evaluated.

     NOTE: The expressions in  l_initv  are  evaluated  from
           left  to  right and then lambda-bound to the sym-
           bols in l_locv.  If there are too few expressions
           in l_initv then the missing values are assumed to
           be nil.  If there are  too  many  expressions  in
           l_initv then the extra ones are ignored (although
           they  are  evaluated).   Then  the   g_exp_i   are
           evaluated  left to right.  The body of a progv is
           like the body of a progn, it is _n_o_t a prog body.

(purcopy 'g_exp)

     RETURNS: a copy of g_exp with new pure cells  allocated
              wherever possible.

     NOTE: pure space is never swept up by the garbage  col-
           lector,  so  this  should only be done on expres-
           sions which are not likely to become  garbage  in
           the  future.   In  certain cases, data objects in
           pure space become read-only after a _d_u_m_p_l_i_s_p  and
           then  an attempt to modify the object will result
           in an illegal memory reference.

(purep 'g_exp)

     RETURNS: t iff the object g_exp is in pure space.

(putd 's_name 'u_func)

     RETURNS: u_func

     SIDE EFFECT: this sets the function binding  of  symbol
                  s_name to u_func.








9

9                                 Printed: September 21, 1982







   Special Functions                                    4-19


(return ['g_val])

     RETURNS: g_val (or nil if g_val is  not  present)  from
              the enclosing prog or do body.

     NOTE: this form is only valid in the context of a  prog
           or do body.

(setarg 'x_argnum 'g_val)

     WHERE:   x_argnum is greater than zero and less than or
              equal to the number of arguments to the lexpr.

     RETURNS: g_val

     SIDE EFFECT: the lexpr's x_argnum'th argument is set to
                  g-val.

     NOTE: this can only be used within the body of a lexpr.

(throw 'g_val [s_tag])

     WHERE:   if s_tag is not given, it  is  assumed  to  be
              nil.

     RETURNS: the value of (*_t_h_r_o_w '_s__t_a_g '_g__v_a_l).

(*throw 's_tag 'g_val)

     RETURNS: g_val from the first enclosing catch with  the
              tag s_tag or with no tag at all.

     NOTE: this is used in conjunction with *_c_a_t_c_h to  cause
           a clean jump to an enclosing context.

(unwind-protect g_protected [g_cleanup1 ...])

     RETURNS: the result of evaluating g_protected.

     NOTE: Normally g_protected is evaluated and  its  value
           remembered, then the g_cleanup_i are evaluated and
           finally  the  saved  value  of   g_protected   is
           returned.    If   something  should  happen  when
           evaluating g_protected which  causes  control  to
           pass  through  g_protected   and thus through the
           call to the unwind-protect, then  the  g_cleanup_i
           will  still  be  evaluated.   This  is  useful if
           g_protected does  something sensitive which  must
           be  cleaned  up  whether  or not g_protected com-
           pletes.


9

9                                 Printed: September 21, 1982



