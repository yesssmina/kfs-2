#ifndef SHELL_H
#define SHELL_H

/* Print the colored prompt on the current screen */
void	shell_prompt(void);

/* Read keys forever: F1..F4 switch screens, other keys feed the shell */
void	shell_run(void);

#endif
