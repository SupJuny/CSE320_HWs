#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/user.h>

#include "deet.h"

struct sigaction new;
sigset_t sig;
int deet_id = 0;
int target_id = 0;
int initial_sig = 0;
int count = 0;
int exec_fail = 0;

typedef struct {
	int valid;
	int d_id;
	int p_id;
	char trace;
	PSTATE upper_pstate;
	char* lower_pstate;
	char** command;
	int num_command;
	int err_status;
}deet_struct;

deet_struct d_id_set[1000];

int evaluate_input(char* str, int argc);
void rm_newline(char* str);
void argv_generate(char* input, char** argv);
void print_output(deet_struct target, int err_no);
void kill_log();

void signal_handler(int sign, siginfo_t* info, void* dump) {
	if (sign == SIGINT) {
		log_signal(SIGINT);
		log_signal(SIGCHLD);

		for (int p = 0; p < deet_id; p++) {
			if (d_id_set[p].upper_pstate != PSTATE_DEAD && d_id_set[p].upper_pstate != PSTATE_NONE) {
    			target_id = p;
    			// kill_log();
    			log_state_change(d_id_set[p].p_id, d_id_set[p].upper_pstate, PSTATE_KILLED, d_id_set[p].err_status);
	    		d_id_set[p].upper_pstate = PSTATE_KILLED;
    			d_id_set[p].lower_pstate = strdup("killed");
    		}
		}
		for (int p = 0; p < deet_id; p++) {
			if (d_id_set[p].upper_pstate != PSTATE_DEAD && d_id_set[p].upper_pstate != PSTATE_NONE) {
				print_output(d_id_set[p], d_id_set[p].err_status);
			}
		}

		for (int p = 0; p < deet_id; p++) {
			if (d_id_set[p].upper_pstate != PSTATE_DEAD && d_id_set[p].upper_pstate != PSTATE_NONE) {
    			kill(d_id_set[p].p_id, SIGKILL);
    			target_id = p;
    			sigemptyset(&sig);
				sigdelset(&sig, SIGCHLD);
				sigsuspend(&sig);
			}
		}
		log_shutdown();
		exit(0);
	}

	else if (sign == SIGCHLD) {
		deet_struct target = d_id_set[target_id];

		// cont exit
		if (info->si_code == CLD_EXITED) {
			if (info->si_status > 254) {
				log_state_change(getpid(), PSTATE_NONE, PSTATE_RUNNING, d_id_set[target_id].err_status);
    			d_id_set[target_id].d_id = deet_id;
    			d_id_set[target_id].p_id = getpid();
    			d_id_set[target_id].upper_pstate = PSTATE_RUNNING;
		    	d_id_set[target_id].lower_pstate = strdup("running");
		    	exec_fail = 1;
				print_output(d_id_set[target_id], d_id_set[target_id].err_status);
			}
			log_signal(SIGCHLD);
			log_state_change(d_id_set[target_id].p_id, d_id_set[target_id].upper_pstate, PSTATE_DEAD, info->si_status);
			d_id_set[target_id].upper_pstate = PSTATE_DEAD;
			free(d_id_set[target_id].lower_pstate);
    		d_id_set[target_id].lower_pstate = strdup("dead");
    		d_id_set[target_id].err_status = info->si_status;
			print_output(d_id_set[target_id], info->si_status);
			d_id_set[target_id].valid = 0;
		}

		// kill or quit exit
		if (info->si_code == CLD_KILLED) {
			log_signal(SIGCHLD);
			log_state_change(target.p_id, target.upper_pstate, PSTATE_DEAD, info->si_status);
			d_id_set[target_id].upper_pstate = PSTATE_DEAD;
			free(d_id_set[target_id].lower_pstate);
			d_id_set[target_id].lower_pstate = strdup("dead");
			d_id_set[target_id].err_status = info->si_status;
    		print_output(d_id_set[target_id], info->si_status);
    		d_id_set[target_id].valid = 0;
		}

		// sigstop exit
		if (info->si_code == CLD_TRAPPED) {
			if(initial_sig) {
				for (int d = 0; d <= deet_id; d++) {
					if (d_id_set[d].lower_pstate == NULL) {
						d_id_set[d].upper_pstate = PSTATE_RUNNING;
				    	d_id_set[d].lower_pstate = strdup("running");
					}
					if (!strcmp(d_id_set[d].lower_pstate, "dead")) {
						log_state_change(d_id_set[d].p_id, d_id_set[d].upper_pstate, PSTATE_NONE, info->si_status);
						d_id_set[d].upper_pstate = PSTATE_NONE;
						free(d_id_set[d].lower_pstate);
						d_id_set[d].lower_pstate = strdup("none");
					}
				}
				d_id_set[target_id].upper_pstate = PSTATE_RUNNING;
				free(d_id_set[target_id].lower_pstate);
		    	d_id_set[target_id].lower_pstate = strdup("running");
				log_state_change(d_id_set[target_id].p_id, PSTATE_NONE, d_id_set[target_id].upper_pstate, info->si_status);
		    	print_output(d_id_set[target_id], info->si_status);
			}

	    	log_signal(SIGCHLD);
			log_state_change(d_id_set[target_id].p_id, d_id_set[target_id].upper_pstate, PSTATE_STOPPED, info->si_status);
			d_id_set[target_id].upper_pstate = PSTATE_STOPPED;
			free(d_id_set[target_id].lower_pstate);
	    	d_id_set[target_id].lower_pstate = strdup("stopped");
			print_output(d_id_set[target_id], info->si_status);
		}

		// stop exit
		if (info->si_code == CLD_STOPPED) {
			log_signal(SIGCHLD);
			log_state_change(d_id_set[target_id].p_id, d_id_set[target_id].upper_pstate, PSTATE_STOPPED, info->si_status);
			d_id_set[target_id].upper_pstate = PSTATE_STOPPED;
			free(d_id_set[target_id].lower_pstate);
	    	d_id_set[target_id].lower_pstate = strdup("stopped");
			print_output(d_id_set[target_id], info->si_status);
		}

		// untrace cont exit
		if (info->si_code == CLD_CONTINUED) {
			log_signal(SIGCHLD);
			log_state_change(d_id_set[target_id].p_id, d_id_set[target_id].upper_pstate, PSTATE_RUNNING, info->si_status);
			d_id_set[target_id].upper_pstate = PSTATE_RUNNING;
			free(d_id_set[target_id].lower_pstate);
	    	d_id_set[target_id].lower_pstate = strdup("running");
		}
	}
}

int set_up(int rm_deet) {
	// char* line_content = NULL;
	char* line_copy = NULL;
	char* input_copy = NULL;
	char **argv;
	char **argv_present;
    size_t n = 0;
    int  num_char = 0;

    // d_id_set = NULL;//(deet_struct*)malloc(sizeof(deet_struct*));
   	// deet_struct* temp;
    // deet_struct process;

    new.sa_sigaction = signal_handler;
    new.sa_flags = SA_SIGINFO;
    sigemptyset(&new.sa_mask);
    sigemptyset(&sig);
    sigaction(SIGINT, &new, NULL);
    sigaction(SIGCHLD, &new, NULL);
    sigaction(SIGSTOP, &new, NULL);

	log_startup();
	log_prompt();

	while(1) {
		char* line_content = NULL;
	    // log_prompt();
	    if (!rm_deet)
	    	fprintf(stdout, "deet> ");

	    num_char = getline(&line_content, &n, stdin);
	    // printf("%s, %d\n", line_content,num_char);
	    if (num_char < 0) {
	    	if (errno == EINTR) {
	    		fflush(stdin);
	    		errno = 0;
	    		clearerr(stdin);
	    		// new.sa_flags = SA_RESTART;
	    		// printf("here\n");
	    		// exit(0);
	    		// return EXIT_SUCCESS;
	    		continue;
	    	}
	    	// printf("out\n");
	    	clearerr(stdin);
	    	return EXIT_SUCCESS;
	    }

	    log_input(line_content);
	    line_copy = strdup(line_content);
	    input_copy = strdup(line_content);

	    char* token = strtok(line_copy, " ");
		int num_str = 0;

		while (token != NULL) {
			num_str++;;
			token = strtok(NULL, " \n");
		}
		// printf("num str is %d\n", num_str);

	    argv = malloc(sizeof(char*) * (num_str + 1));
	    argv_present = malloc(sizeof(char*) * (num_str));

	    argv_generate(input_copy, argv);
	    int input = evaluate_input(input_copy, num_str);

	    switch(input) {
	    	// error
	    	case -1:
	    		rm_newline(line_content);
	    		log_error(argv[0]);
	    		fprintf(stdout, "?\n");
	    		break;

	    	// no input
	    	case -2:
	    		continue;

	    	// help
	    	case 0:
	    		fprintf(stdout, "Available commands:\n");
		        fprintf(stdout, "help -- Print this help message\n");
		        fprintf(stdout, "quit (<=0 args) -- Quit the program\n");
		        fprintf(stdout, "show (<=1 args) -- Show process info\n");
		        fprintf(stdout, "run (>=1 args) -- Start a process\n");
		        fprintf(stdout, "stop (1 args) -- Stop a running process\n");
		        fprintf(stdout, "cont (1 args) -- Continue a stopped process\n");
		        fprintf(stdout, "release (1 args) -- Stop tracing a process, allowing it to continue normally\n");
		        fprintf(stdout, "wait (1-2 args) -- Wait for a process to enter a specified state\n");
		        fprintf(stdout, "kill (1 args) -- Forcibly terminate a process\n");
		        fprintf(stdout, "peek (2-3 args) -- Read from the address space of a traced process\n");
		        fprintf(stdout, "poke (3 args) -- Write to the address space of a traced process\n");
		        fprintf(stdout, "bt (1-2 args) -- Show a stack trace for a traced process\n");
		        break;

		    // quit
	    	case 1:
	    		for (int p = 0; p < deet_id; p++) {
	    			if (d_id_set[p].upper_pstate != PSTATE_DEAD && d_id_set[p].upper_pstate != PSTATE_NONE) {
		    			target_id = p;
		    			// kill_log();
		    			log_state_change(d_id_set[p].p_id, d_id_set[p].upper_pstate, PSTATE_KILLED, d_id_set[p].err_status);
			    		d_id_set[p].upper_pstate = PSTATE_KILLED;
		    			d_id_set[p].lower_pstate = strdup("killed");
		    		}
	    		}
	    		for (int p = 0; p < deet_id; p++) {
	    			if (d_id_set[p].upper_pstate != PSTATE_DEAD && d_id_set[p].upper_pstate != PSTATE_NONE) {
	    				print_output(d_id_set[p], d_id_set[p].err_status);
	    			}
	    		}

	    		for (int p = 0; p < deet_id; p++) {
	    			if (d_id_set[p].upper_pstate != PSTATE_DEAD && d_id_set[p].upper_pstate != PSTATE_NONE) {
		    			kill(d_id_set[p].p_id, SIGKILL);
		    			target_id = p;
		    			sigemptyset(&sig);
						sigdelset(&sig, SIGCHLD);
						sigsuspend(&sig);
					}
				}

	    		log_shutdown();
	    		exit(0);
	    		break;

	    	// show
	    	case 2:
	    		for (int p = 0; p < deet_id; p++) {
	    			if (!strcmp(d_id_set[p].lower_pstate, "none")) {
	    				continue;
	    			}
	    			print_output(d_id_set[p], d_id_set[p].err_status);
	    		}
	    		break;

	    	// run
	    	case 3:
	    		int exit_status;
	    		pid_t pid = fork();
	    		count++;
	    		initial_sig = 1;
	    		argv_present = &argv[1];

	    		// fork failed
	    		if (pid < 0) {
	    			fprintf(stderr, "Fork failed\n");
	    			exit(0);
	    			return 1;
	    		}

	    		// child process
	    		else if(pid == 0) {
	    			// printf("%s \n", argv[0]);
	    			// printf("%s \n", *(argv+1));
	    			dup2(STDERR_FILENO, STDOUT_FILENO);
	    			ptrace(PTRACE_TRACEME, getpid(), NULL, NULL);
	    			// printf("pid in child %d \n", getpid());

	    			if (execvp(argv_present[0], argv_present) < 0) {
	    				d_id_set[target_id].valid = -1;
		    			d_id_set[target_id].d_id = deet_id;
		    			d_id_set[target_id].p_id = getpid();
		    			d_id_set[target_id].upper_pstate = PSTATE_RUNNING;
				    	d_id_set[target_id].lower_pstate = strdup("running");
		    			d_id_set[target_id].command = malloc(sizeof(char*) * (num_str));
		    			d_id_set[target_id].num_command = num_str-1;
		    			for (int i = 0; i < num_str-1; i++) {
		    				d_id_set[target_id].command[i] = strdup(argv_present[i]);
		    			}
						exit(-1);
	    			}
	    		}

	    		// parent process
	    		else {

	    			// empty deet id check
					for (int a = 0; a < deet_id; a++) {
						if (d_id_set[a].valid == 0) {
							target_id = a;
							deet_id--;
							break;
						}
						else {
							// deet_id++;
							target_id = deet_id;
						}
					}

					d_id_set[target_id].valid = 1;
	    			d_id_set[target_id].d_id = target_id;
	    			d_id_set[target_id].p_id = pid;
	    			d_id_set[target_id].trace = 'T';
	    			d_id_set[target_id].command = malloc(sizeof(char*) * (num_str));
	    			d_id_set[target_id].num_command = num_str-1;
	    			for (int i = 0; i < num_str-1; i++) {
	    				d_id_set[target_id].command[i] = strdup(argv_present[i]);
	    			}

	    			waitpid(pid, &exit_status, WUNTRACED);
	    		}
	    		deet_id++;
	    		initial_sig = 0;
	    		break;

	    	// stop
	    	case 4:
	    		int stop_d_id = atoi(argv[1]);
	    		deet_struct target_s = d_id_set[stop_d_id];
	    		target_id = stop_d_id;

	    		if (target_s.upper_pstate != PSTATE_RUNNING)
	    			break;

	    		else {
	    			log_state_change(target_s.p_id, target_s.upper_pstate, PSTATE_STOPPING, target_s.err_status);
		    		d_id_set[stop_d_id].upper_pstate = PSTATE_STOPPING;
		    		free(d_id_set[stop_d_id].lower_pstate);
	    			d_id_set[stop_d_id].lower_pstate = strdup("stopping");
	    			print_output(d_id_set[stop_d_id], target_s.err_status);
	    			kill(target_s.p_id, SIGSTOP);
	    		}
	    		break;

	    	// cont
	    	case 5:
	    		int cont_d_id = atoi(argv[1]);
	    		deet_struct target_c = d_id_set[cont_d_id];
	    		target_id = cont_d_id;

	    		if (target_c.upper_pstate != PSTATE_DEAD && target_c.upper_pstate != PSTATE_NONE && target_c.trace == 'T') {
	    			log_state_change(target_c.p_id, target_c.upper_pstate, PSTATE_RUNNING, target_c.err_status);
		    		d_id_set[cont_d_id].upper_pstate = PSTATE_RUNNING;
		    		free(d_id_set[cont_d_id].lower_pstate);
	    			d_id_set[cont_d_id].lower_pstate = strdup("running");
	    			print_output(d_id_set[cont_d_id], target_c.err_status);
	    		}

	    		if (target_c.upper_pstate != PSTATE_DEAD && target_c.upper_pstate != PSTATE_NONE && target_c.trace == 'U') {
	    			log_state_change(target_c.p_id, target_c.upper_pstate, PSTATE_CONTINUING, target_c.err_status);
		    		d_id_set[cont_d_id].upper_pstate = PSTATE_CONTINUING;
		    		free(d_id_set[cont_d_id].lower_pstate);
	    			d_id_set[cont_d_id].lower_pstate = strdup("continuing");
	    			print_output(d_id_set[cont_d_id], target_c.err_status);
	    		}

    			// traced pid kill
    			if (target_c.trace == 'T') {
    				// printf("traced pid\n");
    				if (ptrace(PTRACE_CONT, target_c.p_id, NULL, NULL) == -1) {
    					fprintf(stderr, "ptrace: No such process\n");
    					break;
    				}
    			}

    			// untraced pid kill
    			else {
    				// printf("untraced pid\n");
    				kill(target_c.p_id, SIGCONT);
    			}
	    		break;

	    	// release
	    	case 6:
	    		int release_d_id = atoi(argv[1]);
	    		deet_struct target_r = d_id_set[release_d_id];
	    		target_id = release_d_id;

	    		log_state_change(target_r.p_id, target_r.upper_pstate, PSTATE_RUNNING, exit_status);
	    		d_id_set[release_d_id].upper_pstate = PSTATE_RUNNING;
	    		free(d_id_set[release_d_id].lower_pstate);
    			d_id_set[release_d_id].lower_pstate = strdup("running");
    			d_id_set[release_d_id].trace = 'U';
    			print_output(d_id_set[release_d_id], exit_status);

    			ptrace(PTRACE_DETACH, target_r.p_id, NULL, NULL);
	    		break;

	    	// wait
	    	case 7:
	    		int wait_d_id = atoi(argv[1]);
	    		target_id = wait_d_id;
	    		char *request;

	    		if (argv[2] == NULL) {
	    			request = strdup("dead");
	    		}
	    		else {
	    			request = strdup(argv[2]);
	    		}
	    		deet_struct target_w = d_id_set[wait_d_id];

	    		while (1) {
	  				if (target_w.lower_pstate == NULL)
	  					;

	    			else if (!strcmp(target_w.lower_pstate, request))
	    				break;
	    		}

	    		break;

	    	// kill
	    	case 8:
	    		int kill_d_id = atoi(argv[1]);
	    		target_id = kill_d_id;

	    		kill_log();
	    		break;

	    	// peek
	    	case 9:
	    		int peek_d_id = atoi(argv[1]);
	    		int arg3_peek = 0;
	    		long address_peek;

	    		if (argv[3] != NULL)
	    			arg3_peek = atoi(argv[3]);

	    		address_peek = strtol(argv[2], NULL, 16);

	    		deet_struct target_pe = d_id_set[peek_d_id];
	    		target_id = peek_d_id;

				long value_peek = ptrace(PTRACE_PEEKDATA, target_pe.p_id, address_peek, NULL);
				if (value_peek == -1) {
	    			fprintf(stderr, "ptrace peek error\n");
	    		}

				if (arg3_peek) {
					for (int i = 0; i < arg3_peek; i++) {
						fprintf(stdout, "%016lx\t%016lx\n", address_peek, value_peek);
						address_peek = address_peek + 0x8;
						value_peek = ptrace(PTRACE_PEEKDATA, target_pe.p_id, address_peek, NULL);
						if (value_peek == -1) {
			    			fprintf(stderr, "ptrace peek error\n");
			    		}
					}
				}
				else {
					fprintf(stdout, "%016lx\t%016lx\n", address_peek, value_peek);
				}
	    		break;

	    	// poke
	    	case 10:
	    		int poke_d_id = atoi(argv[1]);
	    		long arg3_poke = 0;
	    		long address_poke;

	    		address_poke = strtol(argv[2], NULL, 16);
	    		arg3_poke = strtol(argv[3], NULL, 16);
	    		deet_struct target_po = d_id_set[poke_d_id];
	    		target_id = poke_d_id;

	    		long value_poke = ptrace(PTRACE_POKEDATA, target_po.p_id, address_poke, arg3_poke);
	    		if (value_poke == -1) {
	    			fprintf(stderr, "ptrace poke error\n");
	    		}
	    		break;

	    	// bt
	    	case 11:
	    		int bt_d_id = atoi(argv[1]);
	    		long arg2_bt = 0;
	    		long jjin;		// address
	    		long mak;		// value
	    		long orig_address;
	    		long next_address;
	    		deet_struct target_b = d_id_set[bt_d_id];
	    		struct user_regs_struct user;

	    		if (argv[2] != NULL)
	    			arg2_bt = atoi(argv[2]);

	    		ptrace(PTRACE_GETREGS, target_b.p_id, NULL, &user);
	    		jjin = user.rbp;
	    		next_address = ptrace(PTRACE_PEEKDATA, target_b.p_id, jjin, NULL);
	    		if (next_address == -1) {
	    			fprintf(stderr, "ptrace peek error in bt\n");
	    			break;
	    		}
	    		orig_address = jjin + 0x8;
	    		mak = ptrace(PTRACE_PEEKDATA, target_b.p_id, orig_address, NULL);
	    		if (mak == -1) {
	    			fprintf(stderr, "ptrace peek error in bt\n");
	    			break;
	    		}

	    		// given number to print
	    		if (arg2_bt) {
	    			// printf("given\n");
	    			for (int b = 0; b < arg2_bt+1; b++) {
	    				if (next_address == 0x1)
	    					break;
		    			fprintf(stdout, "%016lx\t%016lx\n", jjin, mak);
		    			jjin = next_address;
						next_address = ptrace(PTRACE_PEEKDATA, target_b.p_id, jjin, NULL);
						if (next_address == -1) {
			    			fprintf(stderr, "ptrace peek error in bt - new address\n");
			    			break;
			    		}
			    		orig_address = jjin + 0x8;
			    		mak = ptrace(PTRACE_PEEKDATA, target_b.p_id, orig_address, NULL);
						if (mak == -1) {
			    			fprintf(stderr, "ptrace peek error in bt - mak\n");
			    			break;
			    		}
		    		}
	    		}

	    		// default = 10
	    		else {
	    			// printf("default\n");
	    			for (int b = 0; b < 10; b++) {
	    				if (next_address == 0x1)
	    					break;
		    			fprintf(stdout, "%016lx\t%016lx\n", jjin, mak);
		    			jjin = next_address;
						next_address = ptrace(PTRACE_PEEKDATA, target_b.p_id, jjin, NULL);
						if (next_address == -1) {
			    			fprintf(stderr, "ptrace peek error in bt - new address\n");
			    			break;
			    		}
			    		orig_address = jjin + 0x8;
			    		mak = ptrace(PTRACE_PEEKDATA, target_b.p_id, orig_address, NULL);
						if (mak == -1) {
			    			fprintf(stderr, "ptrace peek error in bt - mak\n");
			    			break;
			    		}
		    		}
	    		}
	    		break;
	    }
	    log_prompt();
	    fflush(stdout);
	    free(line_copy);
	    free(input_copy);
	}

    return 0;
}

int evaluate_input(char* str, int argc) {
	int i, j = 0;

	// get command
	while(1) {
		// printf("hi %c", str[j]);
		// printf("%d\n", str[j]);
		if (str[j] < 97 || str[j] > 122)
			break;
		j++;
	}
	// printf("size of argv is %d\n", argc);

	// get command index
	if (j == 0) i = -2;
	else if (!strncmp(str, "help", 4)) {
		if (j != 4)
			i = -1;
		else
			i = 0;
	}
    else if (!strncmp(str, "quit", 4)) {
    	if (j != 4 || argc > 1)
			i = -1;
		else
			i = 1;
    }
    else if (!strncmp(str, "show", 4)) {
    	if (j != 4 || argc > 2)
			i = -1;
		else
			i = 2;
    }
    else if (!strncmp(str, "run", 3)) {
    	if (j != 3 || argc < 2)
			i = -1;
		else
    		i = 3;
    }
    else if (!strncmp(str, "stop", 4)) {
    	if (j != 4 || argc != 2)
			i = -1;
		else
    		i = 4;
    }
    else if (!strncmp(str, "cont", 4)) {
    	if (j != 4 || argc != 2)
			i = -1;
		else
    		i = 5;
    }
    else if (!strncmp(str, "release", 7)) {
    	if (j != 7 || argc != 2)
			i = -1;
		else
			i = 6;
    }
    else if (!strncmp(str, "wait", 4)) {
    	if (j != 4 || argc > 3 || argc < 2)
			i = -1;
		else
			i = 7;
    }
    else if (!strncmp(str, "kill", 4)) {
    	if (j != 4 || argc != 2)
			i = -1;
		else
			i = 8;
    }
    else if (!strncmp(str, "peek", 4)) {
    	if (j != 4 || (argc > 4 && argc < 3))
			i = -1;
		else
			i = 9;
    }
    else if (!strncmp(str, "poke", 4)) {
    	if (j != 4 || argc != 4)
			i = -1;
		else
			i = 10;
    }
    else if (!strncmp(str, "bt", 2)) {
    	if (j != 2 || argc > 3 || argc < 2)
			i = -1;
		else
			i = 11;
    }
    else i = -1;

    return i;
}

void rm_newline(char* str) {
	// printf("str in rm function: %s\n", str);
	int j = strlen(str);
	for (int p = 0; p < j; p++)
		if (str[p] == '\n')
			str[p] = '\0';
	// printf("str after in rm function: %s\n", str);
}

void argv_generate(char* input, char** argv) {
	rm_newline(input);

	char* token = strtok(input, " ");
	int i = 0;

	while (token != NULL) {
		// printf("token is %s and length is %d\n", token, (int)strlen(token));
		argv[i] = malloc(sizeof(char) * ((int)strlen(token))+1);
		strcpy(argv[i], token);
		token = strtok(NULL, " ");
		// printf("argv is %s\n", argv[i]);
		i++;
	}
	argv[i] = NULL;

	// printf("!!!%s\n", argv[1]);
}

void print_output(deet_struct target, int err_no) {
	// printf("p state is %d\n", target.upper_pstate);
	fprintf(stdout, "%d\t %d\t %c\t", target.d_id, target.p_id, target.trace);
	if(!strcmp(target.lower_pstate, "dead")) {
		if (exec_fail == 1) {
			fprintf(stdout, "%s\t0x100\t", target.lower_pstate);
			exec_fail = 0;
		}
		else {
			fprintf(stdout, "%s\t0x%x\t", target.lower_pstate, err_no);
		}
	}
	else {
		fprintf(stdout, "%s\t\t", target.lower_pstate);
	}
	for (int a = 0; a < target.num_command; a++) {
		fprintf(stdout, "%s ", target.command[a]);
	}
	fprintf(stdout, "\n");
}

void kill_log() {
	deet_struct target = d_id_set[target_id];
	// printf("pstate in kill function is %d\n", target.upper_pstate);

	if (target.upper_pstate != PSTATE_DEAD && target.upper_pstate != PSTATE_NONE) {
		log_state_change(target.p_id, target.upper_pstate, PSTATE_KILLED, target.err_status);
		d_id_set[target_id].upper_pstate = PSTATE_KILLED;
		free(d_id_set[target_id].lower_pstate);
		d_id_set[target_id].lower_pstate = strdup("killed");
		print_output(d_id_set[target_id], d_id_set[target_id].err_status);


		kill(target.p_id, SIGKILL);

		sigfillset(&sig);
		sigdelset(&sig, SIGCHLD);
		sigsuspend(&sig);
	}
}
