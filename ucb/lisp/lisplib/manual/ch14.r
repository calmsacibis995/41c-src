






                        CCCCHHHHAAAAPPPPTTTTEEEERRRR  11114444


                      TTTThhhheeee LLLLIIIISSSSPPPP SSSStttteeeeppppppppeeeerrrr







11114444....1111....  SSSSiiiimmmmpppplllleeee UUUUsssseeee OOOOffff SSSStttteeeeppppppppiiiinnnngggg

((((sssstttteeeepppp s_arg1...))))

     NOTE: The LISP "stepping" package is intended  to  give
           the  LISP  programmer a facility analogous to the
           Instruction  Step  mode  of  running  a   machine
           language  program.  The user interface is through
           the function (fexpr) step, which sets switches to
           put the LISP interpreter in and out of "stepping"
           mode.  The most common _s_t_e_p  invocations  follow.
           These  invocations  are usually typed at the top-
           level, and will take effect immediately (i.e. the
           next  S-expression  typed in will be evaluated in
           stepping mode).


    ____________________________________________________

    (_s_t_e_p _t)                                ; Turn on stepping mode.
    (_s_t_e_p _n_i_l)                      ; Turn off stepping mode.
    ____________________________________________________




     SIDE EFFECT: In stepping mode, the LISP evaluator  will
                  print  out  each  S-exp  to  be  evaluated
                  before evaluation, and the returned  value
                  after  evaluation,  calling  itself recur-
                  sively to display the  stepped  evaluation
                  of  each argument, if the S-exp is a func-
                  tion call.  In stepping mode, the  evalua-
                  tor  will wait after displaying each S-exp
                  before evaluation for a command  character
                  from the console.





9

9TTTThhhheeee LLLLIIIISSSSPPPP SSSStttteeeeppppppppeeeerrrr                                        11114444----1111







TTTThhhheeee LLLLIIIISSSSPPPP SSSStttteeeeppppppppeeeerrrr                                        11114444----2222



    ____________________________________________________

    _S_T_E_P _C_O_M_M_A_N_D _S_U_M_M_A_R_Y

    <return>                        Continue stepping recursively.

    c                               Show returned value from this level
                                    only, and continue stepping upward.

    e                               Only step interpreted code.

    g                               Turn off stepping mode. (but continue
                                    evaluation without stepping).

    n <number>                      Step through <number> evaluations without
                                    stopping

    p                               Redisplay current form in full
                                    (i.e. rebind prinlevel and prinlength to nil)

    b                               Get breakpoint

    q                               Quit

    d                               Call debug
    ____________________________________________________







     11114444....2222....  AAAAddddvvvvaaaannnncccceeeedddd FFFFeeeeaaaattttuuuurrrreeeessss




     11114444....2222....1111....  SSSSeeeelllleeeeccccttttiiiivvvveeeellllyyyy TTTTuuuurrrrnnnniiiinnnngggg OOOOnnnn SSSStttteeeeppppppppiiiinnnngggg....

     If
            (_s_t_e_p _f_o_o_1 _f_o_o_2 ...)

     is typed at  top  level,  stepping  will  not  commence
     immediately,   but  rather  when  the  evaluator  first
     encounters an S-expression whose car is  one  of  _f_o_o_1,
     _f_o_o_2, etc.  This form will then display at the console,
     and the evaluator will be in stepping mode waiting  for
     a command character.

          Normally the stepper intercepts calls  to  _f_u_n_c_a_l_l
     and  _e_v_a_l.   When _f_u_n_c_a_l_l is intercepted, the arguments


                                     Printed: March 23, 1982







TTTThhhheeee LLLLIIIISSSSPPPP SSSStttteeeeppppppppeeeerrrr                                        11114444----3333


     to the function have already been  evaluated  but  when
     _e_v_a_l  is  intercepted,  the  arguments  have  not  been
     evaluated.  To differentiate the two cases, when print-
     ing the form in evaluation, the stepper preceded inter-
     cepted calls to _f_u_n_c_a_l_l with "f:".   Calls  to  _f_u_n_c_a_l_l
     are normally caused by compiled lisp code calling other
     functions, whereas calls to  _e_v_a_l  usually  occur  when
     lisp  code  is interpreted.  To step only calls to eval
     use:         (_s_t_e_p _e)





     11114444....2222....2222....  SSSStttteeeeppppppppiiiinnnngggg WWWWiiiitttthhhh BBBBrrrreeeeaaaakkkkppppooooiiiinnnnttttssss....

          For the moment, step is turned off inside of error
     breaks,  but  not  by the break function.  Upon exiting
     the error, step is reenabled.  However, executing (_s_t_e_p
     _n_i_l)  inside  a  error loop will turn off stepping glo-
     bally, i.e. within the error loop, and after return has
     be made from the loop.




     11114444....3333....  OOOOvvvveeeerrrrhhhheeeeaaaadddd ooooffff SSSStttteeeeppppppppiiiinnnngggg....

          If stepping mode has  been  turned  off  by  (_s_t_e_p
     _n_i_l),  the  execution  overhead  of having the stepping
     packing in your LISP is identically nil.  If one  stops
     stepping  by  typing  "g",  every call to eval incurs a
     small    overhead--several    machine     instructions,
     corresponding  to  the  compiled code for a simple cond
     and one function pushdown.   Running  with  (_s_t_e_p  _f_o_o_1
     _f_o_o_2  ...) can be more expensive, since a member of the
     car of the current form into the list (_f_o_o_1  _f_o_o_2  ...)
     is required at each call to eval.




     11114444....4444....  EEEEvvvvaaaallllhhhhooooooookkkk aaaannnndddd FFFFuuuunnnnccccaaaallllllllhhhhooooooookkkk

          There are hooks in the FRANZ LISP  interpreter  to
     permit  a  user written function to gain control of the
     evaluation process.  These hooks are used by  the  Step
     package  just  described.  There are two hooks and they
     have been strategically placed in the two key functions
     in  the  interpreter:  _e_v_a_l (which all interpreted code
     goes through) and _f_u_n_c_a_l_l (which all compiled code goes
     through if (_s_s_t_a_t_u_s _t_r_a_n_s_l_i_n_k _n_i_l) has been done).  The
     hook in _e_v_a_l is compatible with Maclisp, but  there  is


                                     Printed: March 23, 1982







TTTThhhheeee LLLLIIIISSSSPPPP SSSStttteeeeppppppppeeeerrrr                                        11114444----4444


     no Maclisp equivalent of the hook in _f_u_n_c_a_l_l.

          To arm the hooks  two  forms  must  be  evaluated:
     (*_r_s_e_t _t) and (_s_s_t_a_t_u_s _e_v_a_l_h_o_o_k _t).  Once that is done,
     _e_v_a_l and _f_u_n_c_a_l_l do a special check when they enter.

          If  _e_v_a_l  is  given  a  form  to   evaluate,   say
     (_f_o_o _b_a_r),  and  the  symbol `evalhook' is non nil, say
     its value is `ehook', then _e_v_a_l will  lambda  bind  the
     symbols  `evalhook'  and  `funcallhook' to nil and will
     call ehook passing (_f_o_o _b_a_r) as the  argument.   It  is
     ehook's responsibility to evaluate (_f_o_o _b_a_r) and return
     its value.  Typically  ehook  will  call  the  function
     `evalhook' to evaluate (_f_o_o _b_a_r).  Note that `evalhook'
     is a symbol whose function binding is a system function
     described in Chapter 4, and whose value binding, if non
     nil, is the name of  a  user  written  function  (or  a
     lambda  expression, or a binary object) which will gain
     control whenever eval is called.   `evalhook'  is  also
     the name of the _s_t_a_t_u_s tag which must be set for all of
     this to work.

          If _f_u_n_c_a_l_l is given a function, say foo, and a set
     of  already evaluated arguments, say barv and bazv, and
     if the symbol `funcallhook' has a non  nil  value,  say
     `fhook',  then  _f_u_n_c_a_l_l will lambda bind `evalhook' and
     `funcallhook' to nil and will call fhook with arguments
     barv,  bazv  and foo.  Thus fhook must be a lexpr since
     it may be given any number of arguments.  The  function
     to  call,  foo  in  this  case, will be the _l_a_s_t of the
     arguments given to fhook.  It is fhooks  responsibility
     to  do  the  function call and return the value.  Typi-
     cally fhook will call the function  _f_u_n_c_a_l_l_h_o_o_k  to  do
     the funcall.  This is an example of a funcallhook func-
     tion which  just prints the arguments on each entry  to
     funcall and the return value.
















9

9                                     Printed: March 23, 1982







TTTThhhheeee LLLLIIIISSSSPPPP SSSStttteeeeppppppppeeeerrrr                                        11114444----5555



    ____________________________________________________

    -> (_d_e_f_u_n _f_h_o_o_k _n (_l_e_t ((_f_o_r_m (_c_o_n_s (_a_r_g _n) (_l_i_s_t_i_f_y (_1- _n))))
                            (_r_e_t_v_a_l))
                           (_p_a_t_o_m "_c_a_l_l_i_n_g ")(_p_r_i_n_t _f_o_r_m)(_t_e_r_p_r)
                           (_s_e_t_q _r_e_t_v_a_l (_f_u_n_c_a_l_l_h_o_o_k _f_o_r_m '_f_h_o_o_k))
                           (_p_a_t_o_m "_r_e_t_u_r_n_s ")(_p_r_i_n_t _r_e_t_v_a_l)(_t_e_r_p_r)
                           _r_e_t_v_a_l))
    fhook
    -> (*_r_s_e_t _t) (_s_s_t_a_t_u_s _e_v_a_l_h_o_o_k _t) (_s_s_t_a_t_u_s _t_r_a_n_s_l_i_n_k _n_i_l)
    -> (_s_e_t_q _f_u_n_c_a_l_l_h_o_o_k '_f_h_o_o_k)
    calling (print fhook)           ;; now all compiled code is traced
    fhookreturns nil
    calling (terpr)

    returns nil
    calling (patom "-> ")
    -> returns "-> "
    calling (read nil Q00000)
    (_a_r_r_a_y _f_o_o _t _1_0)                ;; to test it, we see what happens when
    returns (array foo t 10)        ;; we make an array
    calling (eval (array foo t 10))
    calling (append (10) nil)
    returns (10)
    calling (lessp 1 1)
    returns nil
    calling (apply times (10))
    returns 10
    calling (small-segment value 10)
    calling (boole 4 137 127)
    returns 128
     ... there is plenty more ...
    ____________________________________________________


















9

9                                     Printed: March 23, 1982



