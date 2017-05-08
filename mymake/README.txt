mymake:
	mymake behaves like unix make, mymake checking all dependencies.
	(and the timestamps of the related files) in the makefile and performing operations only when needed.

	mymake supports two components in the makefile: macros and target rules.

	mymake supports comment (#)
	
	mymake supports circular dependencies in the makefile.
	
	mymake supports the command line in four following forms:
		1. a simple command with command line arguments
		2. multiple commands separated by ';'.(These commands are to be executed sequentially in order.)
		3. the 'cd' command (the effect of cd is only on one line of multiple commands).
		4. a command to be executed in background ('&').
		5. a command with redirected I/O ('>' and '<').

	mymake supports the following options.
		-f mf : replace the default makefiles with file 'mf'. Without this option, 
		          the program will search for the default mymake1.mk, mymake2.mk,
		          mymake3.mk in the current directory in order.

		-p : with this option, mymake should build the rules database from the makefile, 
		      output the rules and exit.
	
		-d : with this option, mymake should print debugging information while executing.
		      The debugging information should include the rules applied, and the actions 
		      (commands) executed. See the sample executable for more details.

		-i : with this option, mymake should block the SIGINT signal so that Ctrl-C would not 
		    have effect on the program. Without this option, the default behavior is that mymake 
		    will clean up (kill) all of the children that it created and then exit when Ctrl-C is typed.

		-t num : with this option, mymake should run for up to roughly num seconds. If the program 
		             does not finish in num seconds, it should gracefully self-destruct (clean up all of its 
		             children and then exit).

	
	
		
