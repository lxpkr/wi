/*
 * +--------------------------------------------------------------------+
 * | Function: procs.c                                   Date: 92/04/04 |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Gets the proc info.                                         |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *(1) 2.3: All the graphic shit and the detial screen can now be
 *		watched.
 *							Sun Aug 22 13:28:04 EDT 1993 - PKR.
 * Bugs:
 *          None yet.
 */

#include "wi.h"

#include <pwd.h>
#include <grp.h>
#include <sys/dir.h>
#include <sys/var.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/user.h>
#include <sys/immu.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <signal.h>

#define START_OF_NICE 4

extern int size_flag;
int    pg_ctr;
int max_procs_to_display;
extern int nprocs;

char dev_name[32];

void wi_procs(void);
void getproc(int i, int pg_ctr);
void proc_header(void);
char *get_cpu_time(time_t ticks);
void detial_proc_menu(int i);
void detial_proc(int i, WINDOW *tmp_window);
void detial_proc_head(WINDOW *tmp_window);
void watch_procs(void);
void proc_mon_header(void);

void draw_process_screen(int slot, int max_val);
void watch_processes(int *main_slot, int max_val);

void nice_proc(register int slot);
void get_new_nice_value(int *new_nice);
void num_of_open_procs(void);
static void sig_alrm_proc(int signo);


extern proc_t *current_procs;
extern proc_t **ptr_procs;

int procs_per_pstat[SXBRK + 1];
int procs_in_core;
int procs_alive;

/*
 * +--------------------------------------------------------------------+
 * | Function:                                           Date: %D% |
 * |                                                                    |
 * | Notes:                                                             |
 * |                                                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

#ifdef __STDC__
#	pragma comment(exestr, "%Z% %M%		Version %I% %D% - Dapix ")
#else
#	ident "%Z% %M%		Version %I% %D% - Dapix "
#endif /* __STDC__ */


void wi_procs(void)
{

int	i;
int	get_value = 0;
register max_val;

	procs = (proc_t *) malloc(v.v_proc * sizeof(proc_t));
	kmem_read(procs, namelist[NM_PROC].n_value, sizeof(proc_t) * v.v_proc);

	proc_header();
	max_val = v.v_proc;
	pg_ctr = 4;
	for (i = 0; ;) {
		if (pg_ctr == page_len) {
			wnoutrefresh(main_win);
			an_option();
			num_of_open_procs();
			doupdate();

			switch(wgetch(bottom_win)) {
			case 'w' :
				if (size_flag)
					if (i <= 31)
						i = 0;
					else
						i -= 31;
				else
					if (i <= 13)
						i = 0;
					else
						i -= 13;
				drawdisp(27);
				watch_processes(&i, max_val);
				drawdisp(9);
				break;

			case 'b' :
			case 'B' :
			case KEY_PPAGE:
				if (size_flag) {
					if (i <= 61) {
						beep();
						i = 0;
					} else
						i -= 62;
				} else {
					if (i <= 25) {
						beep();
						i = 0;
					} else
						i -= 26;
				}
				break;

			case 'f' :
			case 'F' :
			case KEY_NPAGE:
				if (size_flag) {
					if (i >= (max_val - 30)) {
						i = (max_val - 31);
						beep();
					}
				} else {
					if (i >= (max_val - 12)) {
						i = (max_val - 13);
						beep();
					}
				}
				break;

			case KEY_UP:
			case '-' :
				if (size_flag) {
					if (i <= 32) {
						beep();
						i = 0;
					} else
						i -= 32;
				} else {
					if (i <= 13) {
						beep();
						i = 0;
					} else
						i -= 14;
				}
				break;

			case KEY_DOWN:
			case '+' :
				if (i == (max_val)) {
					if (size_flag)
						i = (max_val - 31);
					else
						i = (max_val - 13);
					beep();
				} else {
					if (size_flag) {
						if (i <= 30)
							i = 0;
						else
							i -= 30;
					} else {
						if (i <= 12)
							i = 0;
						else
							i -= 12;
					}
				}
				break;

			case 'd' :
			case 'D' :
				drawdisp(9);
				Set_Colour(main_win, Colour_Banner);
				mvwaddstr(main_win, 0, 46, "Detail");
				Set_Colour(main_win, Normal);
				wrefresh(main_win);
				get_value = 0;
				get_detial(&get_value, 0, (max_val - 1));
				doupdate();
				(void) detial_proc_menu(get_value);
				drawdisp(9);
				wrefresh(main_win);
				if (size_flag)
					if (i <= 31)
						i = 0;
					else
						i -= 31;
				else
					if (i <= 13)
						i = 0;
					else
						i -= 13;
				break;

#ifdef CHANGE_NICE
			case 'n' :
			case 'N' :
				if (nice_enabled) {
					get_value = 0;
					if (!(get_item(&get_value, START_OF_NICE, (max_val - 1)))) {
						(void) nice_proc(get_value);
						kmem_read(procs, namelist[NM_PROC].n_value,
				    			sizeof(proc_t) * v.v_proc);
					}
				} else
					beep();
				if (size_flag)
					if (i <= 31)
						i = 0;
					else
						i -= 31;
				else
					if (i <= 13)
						i = 0;
					else
						i -= 13;
				break;
#endif
			case 's' :
			case 'S' :
				drawdisp(9);
				Set_Colour(main_win, Colour_Banner);
				mvwaddstr(main_win, 0, 70, "Search");
				Set_Colour(main_win, Normal);
				wrefresh(main_win);
				get_value = 0;
				get_item(&get_value, 0, (max_val - 1));
				i = get_value;
				drawdisp(9);
				wrefresh(main_win);
				break;

			case 'q' :
			case 'Q' :
				free((char *) procs);
				return;

			case 'W' :
				drawdisp(15);
				if (size_flag)
					max_procs_to_display = 31;
				else
					max_procs_to_display = 13;
				watch_procs();
				drawdisp(9);
				break;

			case 'L' & 0x1F:
			case 'R' & 0x1F:
				clear_the_screen();
				if (size_flag)
					if (i <= 31)
						i = 0;
					else
						i -= 31;
				else
					if (i <= 13)
						i = 0;
					else
						i -= 13;
				break;

			case 'P' :
			case 'p' :
				screen_dump();
				if (size_flag)
					if (i <= 31)
						i = 0;
					else
						i -= 31;
				else
					if (i <= 13)
						i = 0;
					else
						i -= 13;
				break;

			case 'u' :
			case 'U' :
				kmem_read(procs, namelist[NM_PROC].n_value,
				    sizeof(proc_t) * v.v_proc);

			default :

				if (size_flag)
					if (i <= 31)
						i = 0;
					else
						i -= 31;
				else
					if (i <= 13)
						i = 0;
					else
						i -= 13;
				break;

			}
			proc_header();
			pg_ctr = 4;
		} else {
			getproc(i, pg_ctr);
			if (i == (max_val)) {
				pg_ctr++;
				i = 0;
			} else
				i++;
			pg_ctr++;
		}
	}
}

/*
 * +--------------------------------------------------------------------+
 * | Function: getproc()                                 Date: 92/04/04 |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Displays the proc info.                                     |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void getproc(int i, int pg_ctr)
{

proc_t   *pp;

	pp = &procs[i];

	mvwaddstr(main_win,pg_ctr, 4, "                                                                         ");

	mvwprintw(main_win, pg_ctr, 3, "%3d -", i);
	set_run_colour(main_win, pp->p_stat);

	if (procs[i].p_stat MATCHES) {
		mvwprintw(main_win,pg_ctr, 9, "Free", i);
		Set_Colour(main_win, Normal);
		return;
	}

	if (pp->p_cpu >= 1 && pp->p_stat == SSLEEP) {
		Set_Colour(main_win, Colour_Ltcyan);
	}
	mvwprintw(main_win, pg_ctr, 7, "%c %3d %3d %5d %5d %3d %0.2d",
	    " sRzdipx"[pp->p_stat], pp->p_cpu & 0377, pp->p_uid,
	    pp->p_pid, pp->p_ppid, pp->p_pri & 0377, pp->p_nice);

	if (pp->p_stat == SZOMB) {
		mvwaddstr(main_win, pg_ctr, 60, "<zombie>");
	} else if (pp->p_stat != 0) {
		if (finduser(pp, i, &user)) {
			mvwprintw(main_win, pg_ctr, 36, "%s",
			    get_cpu_time(user.u_utime));
			mvwprintw(main_win, pg_ctr, 43, "%s",
			    get_cpu_time(user.u_stime));
			mvwprintw(main_win, pg_ctr, 51 - 1, "%5d %3s",
			    (ctob((u_long)user.u_tsize + user.u_dsize + user.u_ssize))/ 1024, print_tty(pp->p_pid, 0));
			mvwprintw(main_win, pg_ctr, 60, "%.17s", user.u_psargs);
		} else {
			switch(pp->p_stat) {
			case SZOMB:
				mvwaddstr(main_win, pg_ctr, 60, "<zombie>");
				break;
			case SXBRK:
				mvwaddstr(main_win, pg_ctr, 60, "<xbreak>");
				break;
			case SIDL:
				mvwaddstr(main_win, pg_ctr, 60, "<in creation>");
				break;
			default :
				Set_Colour(main_win, Colour_Magenta);
				mvwprintw(main_win, pg_ctr, 7, "%c %3d %3d %5d %5d %3d %0.2d",
	    			'S', pp->p_cpu & 0377, pp->p_uid,
	    			pp->p_pid, pp->p_ppid, pp->p_pri & 0377, pp->p_nice);
				mvwaddstr(main_win, pg_ctr, 60, "<swapping>");
			}
		}
	}
	Set_Colour(main_win, Normal);
}

/*
 * +--------------------------------------------------------------------+
 * | Function: getproc(int i, int pg_ctr);               Date: %D% |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Displayes the proc info.                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void proc_header(void)
{

	mvwaddstr(main_win, 2, 2, "                                                                        ");
	mvwaddstr(main_win, 3, 2, "                                                                        ");
	mvwaddstr(main_win, 16, 4, "                                                                       ");
	Set_Colour(main_win, Colour_Blue);
	mvwaddstr(main_win, 2, 2, "Slot");
	mvwaddstr(main_win, 2, 7, "S");
	mvwaddstr(main_win, 2, 9, "CPU");
	mvwaddstr(main_win, 2, 13, "UID");
	mvwaddstr(main_win, 2, 19, "PID");
	mvwaddstr(main_win, 2, 24, "PPID");
	mvwaddstr(main_win, 2, 29, "PRI");
	mvwaddstr(main_win, 2, 33, "NI");
	mvwaddstr(main_win, 2, 36 + 2, "UCPU");
	mvwaddstr(main_win, 2, 43 + 2, "SCPU");
	mvwaddstr(main_win, 2, 51, "SIZE");
	mvwaddstr(main_win, 2, 56, "TTY");
	mvwaddstr(main_win, 2, 60, "CMD");
	Set_Colour(main_win, Normal);
	wnoutrefresh(main_win);
}

/*
 * +--------------------------------------------------------------------+
 * | Function: get_cpu_time(time_t ticks);               Date: %D% |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Gets the CPU time.                                          |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

char *get_cpu_time(time_t ticks)
{

static char time[10];
time_t	mm, ss;

	if (ticks < 0)
		return("------");

	ticks /= HZ;
	mm = ticks / 60L;
	ticks -= mm * 60L;
	ss = ticks;

	if (mm > 9999)
		(void)strcpy(time, ">9999m");
	else if (mm > 999)
		(void)sprintf(time, "%5ldm",mm);
	else
		(void)sprintf(time, "%3lu:%0.2lu",mm,ss);

	return(time);
}

/*
 * +--------------------------------------------------------------------+
 * | Function: detial_proc_head();                       Date: %D% |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Displayes the proc info detial header.                      |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */


void detial_proc_head(WINDOW *win)
{

	Set_Colour(win, Normal);
	mvwaddstr(win, 0, 34, "Lookup");
	mvwaddstr(win, 0, 42, "Time");
	mvwaddstr(win, 0, 48, "Slow");
	mvwaddstr(win, 0, 54, "Med");
	mvwaddstr(win, 0, 59, "Fast");

	Set_Colour(win, Colour_Banner);
	mvwaddstr(win, 0, 2, " Detail Processes ");
	mvwaddstr(win, 0, 34, "L");
	mvwaddstr(win, 0, 42, "T");
	mvwaddstr(win, 0, 48, "S");
	mvwaddstr(win, 0, 54, "M");
	mvwaddstr(win, 0, 59, "F");

	Set_Colour(win, Colour_Blue);

	mvwaddstr(win, 2, 2, "Slot");
	mvwaddstr(win, 2, 8, "State");
	mvwaddstr(win, 2, 15, "Cpu");
	mvwaddstr(win, 2, 20, "Priority");
	mvwaddstr(win, 2, 30, "Nice");
	mvwaddstr(win, 2, 38, "Pid");
	mvwaddstr(win, 2, 44, "Ppid");
	mvwaddstr(win, 2, 51, "Group leader");

	mvwaddstr(win, 5, 2, "ID's");

	mvwaddstr(win, 7, 5, "Control tty");
	mvwaddstr(win, 7, 21, "Wchan");
	mvwaddstr(win, 7, 29, "Sec to Alarm");
	mvwaddstr(win, 7, 44, "Sec to Scheduel");

	mvwaddstr(win, 10, 2, "Start Time");
	mvwaddstr(win, 10, 32, "Text, Data and Stack Size");

	mvwaddstr(win, 12, 2, "Flags");
	mvwaddstr(win, 14, 2, "Command");
	Set_Colour(win, Normal);
	mvwaddstr(win, 10, 12, ":");
	mvwaddstr(win, 5, 6, ":");
	mvwaddstr(win, 12, 7, ":");
	mvwaddstr(win, 14, 9, ":");
	wnoutrefresh(win);

}

/*
 * +--------------------------------------------------------------------+
 * | Function: detial_proc_menu(int proc)                Date: 92/04/04 |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Displays the proc info in detial                            |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void detial_proc_menu(int i)
{

WINDOW *win;

int get_value = 0;
int cmd = 0;
int slot = 0;
int Sleep_Time_Flag = TRUE;
int naptime = 0;
int ffast = FALSE;

	if ( size_flag)
		win = newwin(17, 65, 10, 8);
	else
		win = newwin(17, 65, 5, 8);

	Set_Colour(win, Normal);
	Fill_A_Box(win, BUTTON_BOX);
	Draw_A_Box(win, BUTTON_BOX);
	detial_proc_head(win);
	detial_proc(i, win);

	naptime = Sleep_Time;

	Set_Colour(win, Blink_Banner);
	mvwaddstr(win, 0, 42, "Time");
	Set_Colour(win, Normal);
	wnoutrefresh(top_win);

	while (TRUE) {
		drawdisp(1);
		wnoutrefresh(top_win);
		kmem_read(procs, namelist[NM_PROC].n_value,
			sizeof(proc_t) * v.v_proc);
		detial_proc(i, win);
		num_of_open_procs();
		wnoutrefresh(win);
		an_option();
		doupdate();

		/*
			Set up the signal handler
		*/
		cmd = 0;
		if (ffast == TRUE) {
			nap(400);
			if (rdchk(0))
				cmd = wgetch(bottom_win);
		} else {
			if (signal(SIGALRM, sig_alrm_proc) == SIG_ERR)
				printf("\nsignal(SIGALRM) error\n");

			alarm(naptime);	/* set the alarm timer */
			if ((cmd = wgetch(bottom_win)) < 0) {
				alarm(0);			/* stop the alarm timer */
			} else {
				alarm(0);			/* stop the alarm timer */
			}
		}

		if (cmd != 0) {
			switch(cmd) {
			case -1 :
				break;

			case 'L' & 0x1F:
			case 'R' & 0x1F:
				clear_the_screen();
				break;

			case KEY_PPAGE:
			case KEY_UP:
			case '-' :
				if (i <= 0) {
					i = 0;
					beep();
				} else
					i--;
				break;
	
			case KEY_NPAGE:
			case KEY_DOWN:
			case '+' :
				if (i >= (v.v_proc - 1)) {
					i = (v.v_proc - 1);
					beep();
				} else
					i++;
				break;

			case 'P' :
			case 'p' :
				screen_dump();
				break;

			case 'C' :
			case 'c' :
				ffast = FALSE;
				change_time(FALSE);
				touchwin(win);
				detial_proc_head(win);
				Set_Colour(win, Blink_Banner);
				mvwaddstr(win, 0, 42, "Time");
				Set_Colour(win, Normal);
				wnoutrefresh(win);
				doupdate();
				naptime = Sleep_Time;
				Sleep_Time_Flag = TRUE;
				break;

			case 'l' :
			case 'L' :
				detial_proc_head(win);
				Set_Colour(win, Colour_Banner);
				mvwaddstr(win, 0, 34, "Lookup");
				Set_Colour(win, Normal);
				wrefresh(win);

				get_value = 0;
				get_detial(&get_value, 0, (v.v_proc - 1));
				i = get_value;

				mvwaddstr(win, 0, 34, "Lookup");
				Set_Colour(win, Colour_Banner);
				mvwaddstr(win, 0, 34, "L");
				Set_Colour(win, Blink_Banner);
				if (Sleep_Time_Flag) {
					mvwaddstr(win, 0, 42, "Time");
				} else {
					if (naptime == 4)
						mvwaddstr(main_win, 0, 63, "Slow");
					else if (naptime == 2)
						mvwaddstr(main_win, 0, 68, "Med");
					else if (naptime == 1)
						mvwaddstr(main_win, 0, 72, "Fast");
					else {
						Set_Colour(main_win, Red_Blink_Banner);
						mvwaddstr(main_win, 0, 72, "Fast");
					}
				}
				Set_Colour(win, Normal);
				touchwin(win);
				wnoutrefresh(win);
				break;

			case 'I' :
			case 'i' :
				ffast = FALSE;
				detial_proc_head(win);
				Set_Colour(win, Blink_Banner);
				mvwaddstr(win, 0, 42, "Time");
				Set_Colour(win, Normal);
				naptime = Sleep_Time;
				Sleep_Time_Flag = TRUE;
				break;

			case 'S' :
			case 's' :
				ffast = FALSE;
				detial_proc_head(win);
				Set_Colour(win, Blink_Banner);
				mvwaddstr(win, 0, 48, "Slow");
				Set_Colour(win, Normal);
				naptime = 4;
				Sleep_Time_Flag = FALSE;
				break;

			case 'M' :
			case 'm' :
				ffast = FALSE;
				detial_proc_head(win);
				Set_Colour(win, Blink_Banner);
				mvwaddstr(win, 0, 54, "Med");
				Set_Colour(win, Normal);
				naptime = 2;
				Sleep_Time_Flag = FALSE;
				break;

			case 'F' :
				ffast = TRUE;
				naptime = -1;
				detial_proc_head(win);
				Set_Colour(win, Red_Blink_Banner);
				mvwaddstr(win, 0, 59, "Fast");
				Set_Colour(win, Normal);
				Sleep_Time_Flag = FALSE;
				break;

			case 'f' :
				ffast = FALSE;
				detial_proc_head(win);
				Set_Colour(win, Blink_Banner);
				mvwaddstr(win, 0, 59, "Fast");
				Set_Colour(win, Normal);
				naptime = 1;
				Sleep_Time_Flag = FALSE;
				break;

			case 'Q' :
			case 'q' :
				delwin(win);
				touchwin(main_win);
				wrefresh(main_win);
				return;

			default :
				break;
			}
		}
	}
}


/*
 * +--------------------------------------------------------------------+
 * | Function: detial_proc(int proc)                     Date: 92/04/04 |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Displays the proc info in detial                            |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void detial_proc(int i, WINDOW *win)
{

proc_t   *pp;
struct passwd *pw, *getpwuid();
struct group  *gr, *getgrgid();

char na[] = "N/A";

		mvwaddstr(win, 5, 6, "                                                          ");
		mvwaddstr(win, 8, 2, "                                                          ");
		mvwaddstr(win, 11, 2, "                                                          ");
	if (procs[i].p_stat MATCHES) {

		mvwprintw(win, 3, 2, "%3d    %s   %s    %s      %s    %s    %s       %s", i, na, na, na, na, na, na, na);


		mvwprintw(win, 5, 8, "uid = N/A, suid = N/A, sgid = N/A");
		mvwaddstr(win, 8, 2, "       N/A          N/A        N/A             N/A        ");
		mvwprintw(win, 11, 2, " N/A                            T: N/A, D: N/A, S: N/A     ");
		mvwaddstr(win, 13, 2, " free()                                                   ");

		Set_Colour(win, Text_Warning);
		mvwaddstr(win, 15, 2, " Slot free - ready for use.                                 ");
		Set_Colour(win, Normal);
		wnoutrefresh(win);
		return;
	}

	pp = &procs[i];

	mvwprintw(win, 3, 2, "%3d     %c   %3d     %3d     %3d   %5.1d  %5.1d     %5.1d",
	    i, " sRzdipx"[pp->p_stat], pp->p_cpu & 0377, pp->p_pri & 0377,
	    pp->p_nice, pp->p_pid, pp->p_ppid, pp->p_pgrp);

	mvwprintw(win, 5, 8, "uid = %d(%s), suid = %d(%s), sgid = %d(%s)",
	    pp->p_uid,
	    (((pw = getpwuid(pp->p_uid)) == NULL) ? "unknown": pw->pw_name),
	    pp->p_suid,
	    (((pw = getpwuid(pp->p_suid)) == NULL) ? "unknown": pw->pw_name),
	    pp->p_sgid,
	    (((gr = getgrgid(pp->p_sgid)) == NULL) ? "unknown": gr->gr_name));

	mvwprintw(win, 8, 32, "% 4.1d            % 4.1d", pp->p_time, pp->p_clktim);
	mvwaddstr(win, 8, 4, "                ");

	mvwprintw(win, 11, 3, "%.24s", ctime(&user.u_start));

	if (pp->p_wchan)
		mvwprintw(win, 8, 18, "%8x", pp->p_wchan);
	else if (pp->p_stat == SZOMB) {
		mvwaddstr(win, 3, 10, "z");
		mvwaddstr(win, 8, 18, "  zombie");
	} else
		mvwaddstr(win, 8, 18, " running");

	mvwaddstr(win, 13, 2, "                                                          ");
	wmove(win, 13, 2);

	if (pp->p_stat == SSLEEP)
		waddstr(win, " awaiting an event");
	if (pp->p_stat == SRUN)
		waddstr(win, " running");
	if (pp->p_stat == SZOMB)
		waddstr(win, " zombie");
	if (pp->p_stat == SSTOP)
		waddstr(win, " stop");
	if (pp->p_stat == SIDL)
		waddstr(win, " idle");
	if (pp->p_stat == SXBRK)
		waddstr(win, " xswapped");

	if (pp->p_flag & SSYS)
		waddstr(win, " sys_res");
	if (pp->p_flag & STRC)
		waddstr(win, " traced");
	if (pp->p_flag & SWTED)
		waddstr(win, " waited");
	if (pp->p_flag & SNWAKE)
		waddstr(win, " nwake");
	if (pp->p_flag & SLOAD)
		waddstr(win, " incore");
	if (pp->p_flag & SLOCK)
		waddstr(win, " locked");
	if (pp->p_flag & SPOLL)
		waddstr(win, " str poll");
	if (pp->p_flag & SSLEEP)
		waddstr(win, " sleep()");
	if (pp->p_flag & SEXIT)
		waddstr(win, " exit()");

	if (pp->p_stat == SZOMB) {
		mvwaddstr(win, 15, 2, "                                 ");
		Set_Colour(win, Colour_Magenta);
		Set_Colour(win, Text_Alarm);
		mvwaddstr(win, 15, 2, " zombie");
		Set_Colour(win, Normal);
		mvwaddstr(win, 8, 6, "/dev/??");
	} else if (pp->p_stat != 0) {
		if (finduser(pp, i, &user)) {
			mvwprintw(win, 11, 34, "T: %d, D: %d, S: %d  ",
			    (ctob(user.u_tsize))/ BSIZE,
			    (ctob(user.u_dsize))/ BSIZE,
			    (ctob(user.u_ssize))/ BSIZE);
			mvwaddstr(win, 15, 2, "                                                            ");
			set_run_colour(win, pp->p_stat);
			mvwprintw(win, 15, 2, " %.59s", user.u_psargs);
			Set_Colour(win, Normal);
			if (pp->p_ppid  > 1) {
				print_dev(0, major(user.u_ttyd), minor(user.u_ttyd), dev_name, 0);
				mvwprintw(win, 8, 6, "/dev/%s", dev_name);
			} else
				mvwprintw(win, 8, 6, "/dev/tty%s", print_tty(pp->p_pid, 0));
		} else {
			waddstr(win, " swapped");
			mvwaddstr(win, 8, 6, "/dev/??");
		}
	}

	wnoutrefresh(win);
	return;
}

/*
 * +--------------------------------------------------------------------+
 * | Function:                                           Date: %D% |
 * |                                                                    |
 * | Notes:                                                             |
 * |                                                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void proc_mon_header(void)
{

	mvwaddstr(main_win, 2, 2, "                                                                        ");
	Set_Colour(main_win, Colour_Blue);
	mvwaddstr(main_win, 2, 3, "S");
	mvwaddstr(main_win, 2, 5, "CPU");
	mvwaddstr(main_win, 2, 11, "PID");
	mvwaddstr(main_win, 2, 16, "PPID");
	mvwaddstr(main_win, 2, 21, "PRI");
	mvwaddstr(main_win, 2, 25, "NI");
	mvwaddstr(main_win, 2, 30, "UCPU");
	mvwaddstr(main_win, 2, 37, "SCPU");
	mvwaddstr(main_win, 2, 43, "SIZE");
	mvwaddstr(main_win, 2, 48, "TTY");
	mvwaddstr(main_win, 2, 52, "COMMAND");
	Set_Colour(main_win, Normal);
	wnoutrefresh(main_win);
}

/*
 * +--------------------------------------------------------------------+
 * | Function:                                           Date: %D% |
 * | Author: Paul Ready.                                                |
 * |                                                                    |
 * | Notes:                                                             |
 * |                                                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void watch_procs(void)
{

int get_value = 0;
int cmd = 0;
int Sleep_Time_Flag = TRUE;
int naptime = 0;
int ffast = FALSE;

register ctr, proc_ctr;

	naptime = Sleep_Time;

	proc_mon_header();
	for (ctr = 4; ctr <= page_len; ctr++)
		mvwaddstr(main_win, ctr, 1, "                                                                           ");
	Set_Colour(main_win, Blink_Banner);
	mvwaddstr(main_win, 0, 57, "Time");
	Set_Colour(main_win, Normal);
	wnoutrefresh(top_win);

	if (size_flag)
		max_procs_to_display = 31;
	else
		max_procs_to_display = 12;

	get_sorted_procs();
	max_procs_to_display = min(nprocs,max_procs_to_display);
	for (proc_ctr = 0; proc_ctr < max_procs_to_display; proc_ctr++)
		watch_the_procs(proc_ctr);

	while (TRUE) {
		drawdisp(1);
		wnoutrefresh(top_win);
		for (ctr = 4; ctr <= page_len; ctr++)
			mvwaddstr(main_win, ctr, 1, "                                                                            ");
		if (size_flag)
			max_procs_to_display = 31;
		else
			max_procs_to_display = 12;

		get_sorted_procs();
		max_procs_to_display = min(nprocs,max_procs_to_display);
		for (proc_ctr = 0; proc_ctr < max_procs_to_display; proc_ctr++)
			watch_the_procs(proc_ctr);
		wnoutrefresh(main_win);
		an_option();
		num_of_open_procs();
		doupdate();
		/*
			Set up the signal handler
		*/
		cmd = 0;
		if (ffast == TRUE) {
			nap(400);
			if (rdchk(0))
				cmd = wgetch(bottom_win);
		} else {
			if (signal(SIGALRM, sig_alrm_proc) == SIG_ERR)
				printf("\nsignal(SIGALRM) error\n");

			alarm(naptime);	/* set the alarm timer */
			if ((cmd = wgetch(bottom_win)) < 0) {
				alarm(0);			/* stop the alarm timer */
			} else {
				alarm(0);			/* stop the alarm timer */
			}
		}

		if (cmd != 0) {
			switch(cmd) {
			case -1 :
				break;

			case 'l' :	/* Filter */
			case 'L' :
				drawdisp(15);
				Set_Colour(main_win, Colour_Banner);
				mvwaddstr(main_win, 0, 49, "Filter");
				Set_Colour(main_win, Normal);
				wrefresh(main_win);

				get_value = 0;
				Change_Proc_Filter();
				mvwaddstr(main_win, 0, 49, "Filter");
				Set_Colour(main_win, Colour_Banner);
				mvwaddstr(main_win, 0, 51, "l");
				Set_Colour(main_win, Blink_Banner);
				if (Sleep_Time_Flag) {
					mvwaddstr(main_win, 0, 57, "Time");
				} else {
					if (naptime == 4)
						mvwaddstr(main_win, 0, 63, "Slow");
					else if (naptime == 2)
						mvwaddstr(main_win, 0, 68, "Med");
					else if (naptime == 1)
						mvwaddstr(main_win, 0, 72, "Fast");
					else {
						Set_Colour(main_win, Red_Blink_Banner);
						mvwaddstr(main_win, 0, 72, "Fast");
					}
				}
				Set_Colour(main_win, Normal);
				break;

			case 'C' :
			case 'c' :
				ffast = FALSE;
				change_time(FALSE);
				touchwin(main_win);
				drawdisp(15);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 57, "Time");
				Set_Colour(main_win, Normal);
				doupdate();
				naptime = Sleep_Time;
				Sleep_Time_Flag = TRUE;
				break;

			case 'I' :
			case 'i' :
				ffast = FALSE;
				drawdisp(15);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 57, "Time");
				Set_Colour(main_win, Normal);
				naptime = Sleep_Time;
				Sleep_Time_Flag = TRUE;
				break;

			case 'S' :
			case 's' :
				ffast = FALSE;
				drawdisp(15);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 63, "Slow");
				Set_Colour(main_win, Normal);
				naptime = 4;
				Sleep_Time_Flag = FALSE;
				break;

			case 'M' :
			case 'm' :
				ffast = FALSE;
				drawdisp(15);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 68, "Med");
				Set_Colour(main_win, Normal);
				naptime = 2;
				Sleep_Time_Flag = FALSE;
				break;

			case 'F' :
				ffast = TRUE;
				drawdisp(15);
				Set_Colour(main_win, Red_Blink_Banner);
				mvwaddstr(main_win, 0, 72, "Fast");
				Set_Colour(main_win, Normal);
				Sleep_Time_Flag = FALSE;
				break;

			case 'f' :
				ffast = FALSE;
				drawdisp(15);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 72, "Fast");
				Set_Colour(main_win, Normal);
				naptime = 1;
				Sleep_Time_Flag = FALSE;
				break;

			case 'P' :
			case 'p' :
				screen_dump();
				break;

			case 'Q' :
			case 'q' :
				return;
				break;

			default :
				beep();
				break;
			}
		}
	}
}

/*
 * +--------------------------------------------------------------------+
 * | Function: void Change_Proc_Filter(void)            Date: %Z% |
 * | Author: Paul Ready.                                                |
 * |                                                                    |
 * | Notes:                                                             |
 * |        change the time when the arrow keys are used.               |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

#define ACTIVE 100

int Change_Proc_Filter()
{

extern int sort_type;

extern int Sleep_Time;

	WINDOW *win;

	win = newwin(8, 20, 11, 10);
	Set_Colour(win, Normal);
	Fill_A_Box(win, 0, 0);
	Draw_A_Box(win, BUTTON_BOX);
	mvwaddstr(win, 1, 2, "Sort by Active.");
	mvwaddstr(win, 2, 2, "Sort by Size.");
	mvwaddstr(win, 3, 2, "Sort by SCPU.");
	mvwaddstr(win, 4, 2, "Sort by UCPU.");
	mvwaddstr(win, 5, 2, "Sort by NICE.");
	mvwaddstr(win, 6, 2, "Sort by Priorty.");
	Set_Colour(win, Colour_Banner);
	mvwaddstr(win, 1, 10, "A");
	mvwaddstr(win, 2, 11, "i");
	mvwaddstr(win, 3, 10, "S");
	mvwaddstr(win, 4, 10, "U");
	mvwaddstr(win, 5, 10, "N");
	mvwaddstr(win, 6, 10, "P");

	Set_Colour(win, Normal);

	wnoutrefresh(win);
	an_option();
	doupdate();
	while (TRUE) {
		switch(wgetch(bottom_win)) {
		case 'A' :
		case 'a' :
			sort_type = ACTIVE;
			mvwaddstr(bottom_win, 1, 2, " Watchit proc filter set to ACTIVE.");
			break;

		case 'I' :
		case 'i' :
			sort_type = SIZE;
			mvwaddstr(bottom_win, 1, 2, " Watchit proc filter set to SIZE.");
			break;

		case 'S' :
		case 's' :
			sort_type = SCPU;
			mvwaddstr(bottom_win, 1, 2, " Watchit proc filter set to SCPU.");
			break;

		case 'U' :
		case 'u' :
			sort_type = UCPU;
			mvwaddstr(bottom_win, 1, 2, " Watchit proc filter set to UCPU.");
			break;

		case 'N' :
		case 'n' :
			sort_type = NICE;
			mvwaddstr(bottom_win, 1, 2, " Watchit proc filter set to NICE.");
			break;

		case 'P' :
		case 'p' :
			sort_type = PRI;
			mvwaddstr(bottom_win, 1, 2, " Watchit proc filter set to PRI.");
			break;

		case 'q' :
		case 'Q' :	/* Quit and exit */
		case 27  :
			delwin(win);
			touchwin(main_win);
			return(0);

		default :
			an_option();
			beep();
			continue;
		}

		delwin(win); /* added this */
		touchwin(main_win);
		return;
	}
}

/*
 * +--------------------------------------------------------------------+
 * | Function:                                           Date: %D% |
 * |                                                                    |
 * | Notes:                                                             |
 * |                                                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void draw_process_screen(int slot, int max_val)
{

register ctr;
int pg_ctr;

	kmem_read(procs, namelist[NM_PROC].n_value, sizeof(proc_t) * v.v_proc);

	pg_ctr = 4;
	for (ctr = 0; ;) {
		if (pg_ctr == page_len) {
			wnoutrefresh(main_win);
			return;
		} else {
			getproc(slot, pg_ctr);
			if (slot == (max_val)) {
				pg_ctr++;
				mvwaddstr(main_win, pg_ctr, 2, "                                                                      ");
				slot = 0;
			} else
				slot++;
			pg_ctr++;
		}
	}
}

/*
 * +--------------------------------------------------------------------+
 * | Function: nice_proc(int proc)                       Date: 92/04/04 |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Displays the proc info in detial                            |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void nice_proc(register int slot)
{

extern int no_kmem_write;
proc_t tmp_proc;
int new_nice;
int my_uid;

	if (procs[slot].p_stat MATCHES) {

		mvwaddstr(bottom_win, 1, 2, " : Slot is currently Free, Any key to continue:");
		wclrtoeol(bottom_win);
		Set_Colour(bottom_win, Colour_White);
		mvwaddstr(bottom_win, 1, 2, "Select");
		mvwaddstr(bottom_win, 1, 28, "Free");
		mvwaddstr(bottom_win, 1, 34, "Any key");
		Set_Colour(bottom_win, Normal);
		Draw_A_Box(bottom_win, BUTTON_BOX);
		mvwaddstr(bottom_win, 1, 54, " ");
		wrefresh(bottom_win);
		wgetch(bottom_win);
		touchwin(main_win);
		wrefresh(main_win);
		return;
	}

	my_uid = -1;
	my_uid = getuid();

	kmem_read((caddr_t)&tmp_proc,
		(daddr_t)((proc_t *)namelist[NM_PROC].n_value + slot),
		sizeof(proc_t));

	if (no_kmem_write)
		return;

	if ((my_uid MATCHES) || (my_uid == tmp_proc.p_uid)) {
		new_nice = 20;
		get_new_nice_value(&new_nice);

		tmp_proc.p_nice = new_nice;

		kmem_write((daddr_t)&((proc_t *)namelist[NM_PROC].n_value)[slot] +
			(((caddr_t)&tmp_proc.p_nice) - (caddr_t)&tmp_proc),
			(caddr_t)&tmp_proc.p_nice, sizeof(tmp_proc.p_nice));
	} else {
		beep();
		Set_Colour(bottom_win, Text_Alarm);
		mvwprintw(bottom_win, 1, 2, " Sorry you don't have permission to renice pid: %d - Press <Enter>", tmp_proc.p_pid);
		wclrtoeol(bottom_win);
		Draw_A_Box(bottom_win, BUTTON_BOX);
		mvwaddstr(bottom_win, 1, 69, " ");
		wrefresh(bottom_win);
		wgetch(bottom_win);
		touchwin(main_win);
		wrefresh(main_win);
		return;
	}
	any_key();
	doupdate();
	touchwin(main_win);
	wrefresh(main_win);
	return;
}

/*
 * +--------------------------------------------------------------------+
 * | Function:                                           Date: 93/03/20 |
 * |                                                                    |
 * | Notes:                                                             |
 * |                                                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void get_new_nice_value(int *new_nice)
{

WINDOW *win;
char tmp_val[16];

	tmp_val[0] = '\0';

	win = newwin(3, 46, 14, 12);
	Draw_A_Box(win, BUTTON_BOX);
	Set_Colour(win, Colour_Banner);
	mvwaddstr(win, 0, 1, " nice select ");
	Set_Colour(win, Normal);
	while (TRUE) {
		mvwprintw(win, 1, 25, "                   ");
		mvwprintw(win, 1, 2, "Select a value between %d and %d: ",
			MIN_NICE_VALUE, MAX_NICE_VALUE);
		wrefresh(win);
		wgetstr(win, tmp_val);
		if (tmp_val[0] == 'q') {
			delwin(win);
			touchwin(main_win);
			wrefresh(main_win);
			return;
		}
		*new_nice = atoi(tmp_val);
		if (atoi(tmp_val) >= MIN_NICE_VALUE && atoi(tmp_val) <= MAX_NICE_VALUE) {
			delwin(win);
			touchwin(main_win);
			wrefresh(main_win);
			return;
		}
	}
}

/*
 * +--------------------------------------------------------------------+
 * | Function: void num_of_open_procs(void)                   Date: %Z% |
 * |                                                                    |
 * | Notes:                                                             |
 * |        Draw the header for file                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */


void num_of_open_procs()
{

int proc_ctr  = 0;
proc_t *tmp_procs;
register i;

	/* get proc count */
	tmp_procs = (proc_t *) malloc(v.v_proc * sizeof(proc_t));
	kmem_read(tmp_procs, namelist[NM_PROC].n_value,
	    sizeof(proc_t) * v.v_proc);

	for (i = 0; i < v.v_proc; i++) {
		if (tmp_procs[i].p_stat MATCHES)
			continue;
		proc_ctr++;
	}
	free((char *) tmp_procs);

	Set_Colour(bottom_win, Colour_White);
	mvwprintw(bottom_win, 1, 58, "Procs Running");
	Set_Colour(bottom_win, Normal);
	mvwprintw(bottom_win, 1, 71, ": %3d", proc_ctr);
	mvwaddstr(bottom_win, 1, 28, " ");
	wnoutrefresh(bottom_win);
	return;
}

/*
 *	nothing to do, just return to interrupt the read.
 */

static void sig_alrm_proc(int signo)
{
	return;
}

/*
 * +--------------------------------------------------------------------+
 * | Function:                                           Date: %D% |
 * |                                                                    |
 * | Notes:                                                             |
 * |                                                                    |
 * +--------------------------------------------------------------------+
 *
 * Updates:   
 *          None.
 *    Bugs:
 *          None yet.
 */

void watch_processes(int *main_slot, int max_val)
{

int slot = 0;
int get_value = 0;
int cmd = 0;
int Sleep_Time_Flag = TRUE;
int naptime = 0;
int ffast = FALSE;

	proc_header();

	slot = *main_slot;

	Set_Colour(main_win, Blink_Banner);
	mvwaddstr(main_win, 0, 72, "Fast");
	Set_Colour(main_win, Normal);
	naptime = Sleep_Time;

	while (TRUE) {
		drawdisp(1);
		wnoutrefresh(top_win);
		draw_process_screen(slot, max_val);
		wnoutrefresh(main_win);
		an_option();
		num_of_open_procs();
		doupdate();

		/*
			Set up the signal handler
		*/
		cmd = 0;
		if (ffast == TRUE) {
			nap(400);
			if (rdchk(0))
				cmd = wgetch(bottom_win);
		} else {
			if (signal(SIGALRM, sig_alrm_proc) == SIG_ERR)
				printf("\nsignal(SIGALRM) error\n");

			alarm(naptime);	/* set the alarm timer */
			if ((cmd = wgetch(bottom_win)) < 0) {
				alarm(0);			/* stop the alarm timer */
			} else {
				alarm(0);			/* stop the alarm timer */
			}
		}

		if (cmd != 0) {
			switch(cmd) {
			case -1 :
				break;

			case 'L' & 0x1F:
			case 'R' & 0x1F:
				clear_the_screen();
				break;

			case 'P' :
			case 'p' :
				screen_dump();
				break;

#ifdef CHANGE_NICE
			case 'n' :
			case 'N' :
				if (nice_enabled) {
					get_value = 0;
					if (!(get_item(&get_value, START_OF_NICE, (max_val - 1)))) {
						(void) nice_proc(get_value);
						kmem_read(procs, namelist[NM_PROC].n_value,
				    			sizeof(proc_t) * v.v_proc);
					}
				} else
					beep();
				break;
#endif

			case 'l' :
			case 'L' :
				drawdisp(27);
				Set_Colour(main_win, Colour_Banner);
				mvwaddstr(main_win, 0, 49, "Lookup");
				Set_Colour(main_win, Normal);
				wrefresh(main_win);

				get_value = 0;
				get_item(&get_value, 0, (max_val - 1));
				slot = get_value;

				mvwaddstr(main_win, 0, 49, "Lookup");
				Set_Colour(main_win, Colour_Banner);
				mvwaddstr(main_win, 0, 49, "L");
				Set_Colour(main_win, Blink_Banner);
				if (Sleep_Time_Flag) {
					mvwaddstr(main_win, 0, 57, "Time");
				} else {
					if (naptime == 4)
						mvwaddstr(main_win, 0, 63, "Slow");
					else if (naptime == 2)
						mvwaddstr(main_win, 0, 68, "Med");
					else if (naptime == 1)
						mvwaddstr(main_win, 0, 72, "Fast");
					else {
						Set_Colour(main_win, Red_Blink_Banner);
						mvwaddstr(main_win, 0, 72, "Fast");
					}
				}
				Set_Colour(main_win, Normal);
				break;

			case 'C' :
			case 'c' :
				ffast = FALSE;
				change_time(FALSE);
				touchwin(main_win);
				drawdisp(27);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 57, "Time");
				Set_Colour(main_win, Normal);
				doupdate();
				naptime = Sleep_Time;
				Sleep_Time_Flag = TRUE;
				break;

			case 'I' :
			case 'i' :
				ffast = FALSE;
				drawdisp(27);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 57, "Time");
				Set_Colour(main_win, Normal);
				naptime = Sleep_Time;
				Sleep_Time_Flag = TRUE;
				break;

			case 'S' :
			case 's' :
				ffast = FALSE;
				drawdisp(27);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 63, "Slow");
				Set_Colour(main_win, Normal);
				naptime = 4;
				Sleep_Time_Flag = FALSE;
				break;

			case 'M' :
			case 'm' :
				ffast = FALSE;
				drawdisp(27);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 68, "Med");
				Set_Colour(main_win, Normal);
				naptime = 2;
				Sleep_Time_Flag = FALSE;
				break;

			case 'F' :
				ffast = TRUE;
				naptime = -1;
				drawdisp(27);
				Set_Colour(main_win, Red_Blink_Banner);
				mvwaddstr(main_win, 0, 72, "Fast");
				Set_Colour(main_win, Normal);
				Sleep_Time_Flag = FALSE;
				break;

			case 'f' :
				ffast = FALSE;
				drawdisp(27);
				Set_Colour(main_win, Blink_Banner);
				mvwaddstr(main_win, 0, 72, "Fast");
				Set_Colour(main_win, Normal);
				naptime = 1;
				Sleep_Time_Flag = FALSE;
				break;

			case KEY_PPAGE:
				if (size_flag) {
					if (slot <= 31) {
						beep();
						slot = 0;
					} else
						slot -= 31;
				} else {
					if (slot <= 25) {
						beep();
						slot = 0;
					} else
						slot -= 13;
				}
				break;

			case KEY_NPAGE:
				if (size_flag) {
					if (slot >= (max_val - 61)) {
						slot = (max_val - 31);
						beep();
					} else
						slot += 31;
				} else {
					if (slot >= (max_val - 25)) {
						slot = (max_val - 13);
						beep();
					} else
						slot += 13;
				}
				break;

			case KEY_UP:
			case '-' :
				if (slot MATCHES) {
					beep();
					slot = 0;
				} else
					slot--;
				break;

			case KEY_DOWN:
			case '+' :
				if (size_flag) {
					if (slot >= (max_val - 31)) {
						slot = (max_val - 31);
						beep();
					} else
						slot++;
				} else {
					if (slot >= (max_val - 13)) {
						slot = (max_val - 13);
						beep();
					} else
						slot++;
				}
				break;

			case 'Q' :
			case 'q' :
				*main_slot = slot;
				return;

			default :
				beep();
				break;
			}
		}
	}
}

/*
 * +--------------------------------------------------------------------+
 * |                      END OF THE PROGRAM                            |
 * +--------------------------------------------------------------------+
 */
