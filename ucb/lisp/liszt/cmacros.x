(File cmacros.l)
(Liszt-file macro quote list caddr cadr)
(sfilewriteln macro quote cons cadr)
(sfilewrite macro quote cons cadr)
(forcecomment macro cons quote list cadr)
(makecomment macro cons quote list cadr)
(decr macro quote list cadr)
(incr macro quote list cadr)
(unpush macro quote list cadr)
(Pop macro quote list cadr)
(Push macro quote list caddr cadr)
(niceprint macro cadr quote list cons)
(comp-msg macro list stringp eq car atom cond setq nreverse append quote cons null cdr do)
(comp-gerr macro cdr append quote cons)
(comp-note macro cdr append cons quote list)
(comp-warn macro cdr append cons quote list)
(comp-err macro cdr append quote cons)
