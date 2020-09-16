# Unix Shell written in c
This is a shell I wrote for a class assignment. 

### Features:
Supports shell builtins such as cd and exit.

Supports standard pipe notation with ```|```, and input redirection with ```>```, ```>>``` and ```<```

Supports command chaining and background processes with ```&``` and ```;```

### TODO:
 - Add a command history log so that the up and down arrow may move to past commands like in bash
 - Add arrow key navigation, currently to go back one must press delete
 - ctl+z to pause process and fg to bring it back
 - Add support for ctl-c, currently it will close the program and the shell
