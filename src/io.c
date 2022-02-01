#include <ncurses.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#if ICONS
#include "type.h"
#endif
#include "opts.h"
#include "dir.h"
#include "io.h"


bool print_path = false;
int stdout_back = 0;
size_t n_marked_files = false;


void curses_init(void) {
	if (print_path) {
		stdout_back = dup(STDOUT_FILENO);
		dup2(open("/dev/tty", O_WRONLY), STDOUT_FILENO);
	}

	setlocale(LC_CTYPE, "");

	initscr();
	keypad(stdscr, true);
	curs_set(0);
	noecho();
	raw();
	
	start_color();
	set_color();
}


void terminate_curses(void) {
	keypad(stdscr, false);
	curs_set(1);
	echo();
	noraw();
	endwin();
	
	if (print_path) {
		dup2(stdout_back, STDOUT_FILENO);
	}
}


void set_color(void) {
	if (color) {
		init_color(CUSTOM_DIR, GET_RGB(dir_color));
		init_pair(COLOR_DIR, CUSTOM_DIR, COLOR_BLACK);

		init_color(CUSTOM_LINK, GET_RGB(link_color));
		init_pair(COLOR_LINK, CUSTOM_LINK, COLOR_BLACK);

		init_color(CUSTOM_EXEC, GET_RGB(exec_color));
		init_pair(COLOR_EXEC, CUSTOM_EXEC, COLOR_BLACK);

		init_color(CUSTOM_SOCK, GET_RGB(sock_color));
		init_pair(COLOR_SOCK, CUSTOM_SOCK, COLOR_BLACK);

		init_color(CUSTOM_FIFO, GET_RGB(fifo_color));
		init_pair(COLOR_FIFO, CUSTOM_FIFO, COLOR_BLACK);

		init_color(CUSTOM_BLOCK, GET_RGB(blk_color));
		init_pair(COLOR_BLOCK, CUSTOM_BLOCK, COLOR_BLACK);

		init_color(CUSTOM_UNKNOWN, GET_RGB(unknown_color));
		init_pair(COLOR_UNKNOWN, CUSTOM_UNKNOWN, COLOR_BLACK);

		init_color(CUSTOM_MEDIA, GET_RGB(media_color));
		init_pair(COLOR_MEDIA, CUSTOM_MEDIA, COLOR_BLACK);

		init_color(CUSTOM_ARCHIVE, GET_RGB(archive_color));
		init_pair(COLOR_ARCHIVE, CUSTOM_ARCHIVE, COLOR_BLACK);

		init_color(CUSTOM_FILE, GET_RGB(reg_color));
		init_pair(COLOR_FILE, CUSTOM_FILE, COLOR_BLACK);

		init_pair(RED, COLOR_RED, COLOR_BLACK);
		init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
	} else {
		init_pair(COLOR_DIR, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_LINK, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_EXEC, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_SOCK, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_FIFO, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_UNKNOWN, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_BLOCK, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_MEDIA, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_ARCHIVE, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_FILE, COLOR_WHITE, COLOR_BLACK);
		init_pair(RED, COLOR_WHITE, COLOR_BLACK);
		init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
	}
}


void curses_write_file(struct dir_entry_t * dir_entry, bool highlight) {
	int cp = -1;
	char f_ident;
	char * u_text = "";
#if ICONS
	char * icon = NULL;
#endif
	char * smode = NULL;
	char * owner = NULL;
	char * group = NULL;
	char * size  = NULL;
	char time[128];
	
	if (p_long) {
		smode = mode_to_s(dir_entry);
		owner = getpwuid(dir_entry->owner)->pw_name;
		group = getgrgid(dir_entry->group)->gr_name;
		switch (dir_entry->u_size) {
			case 0: size = "B"; break;
			case 1: size = "KB"; break;
			case 2: size = "MB"; break;
			case 3: size = "GB"; break;
			case 4: size = "TB"; break;
			case 5: size = "PB"; break;
		}
		strftime(time, sizeof(time), "%b %d %H:%M %Y",
				localtime(&dir_entry->mtime));
	}

	switch (dir_entry->file_type) {
		case FILE_DIR:
			cp = COLOR_DIR;
			f_ident = '/';
			break;
		case FILE_FIFO:
			cp = COLOR_FIFO;
			f_ident = '|';
			break;
		case FILE_BLK:
			cp = COLOR_BLOCK;
			f_ident = '#';
			break;
		case FILE_LINK:
			if (dir_entry->under_link == FILE_DIR)
				u_text = "=> /";
			cp = COLOR_LINK;
			f_ident = '@';
			break;
		case FILE_SOCK:
			cp = COLOR_SOCK;
			f_ident = '=';
			break;
		case FILE_UNKNOWN:
			cp = COLOR_UNKNOWN;
			f_ident = '?';
			break;
		default:
			f_ident = NO_IDENT;
			break;
	}

	switch (dir_entry->m_type) {
		case MIME_MEDIA:
			cp = COLOR_MEDIA;
			break;
		case MIME_ARCHIVE:
			cp = COLOR_ARCHIVE;
			break;
		case MIME_UNKNOWN:
		default:
			break;
	}

#if ICONS
	// find icon if it is not a dir
	if (show_icons) {
		icon = get_icon(dir_entry);
	}
#endif

	if ((dir_entry->mode & POWNER(M_EXEC)) &&
			dir_entry->file_type != FILE_LINK &&
			dir_entry->file_type != FILE_DIR) {
		cp = COLOR_EXEC;
		if (f_ident == NO_IDENT) f_ident = '*';
	} else if (cp == -1) {
		cp = COLOR_FILE;
		f_ident = ' ';
	}

	cp = COLOR_PAIR((unsigned)cp);
	if (highlight) cp |= A_REVERSE;

	if (dir_entry->marked) printw("%c ", '-');
	if (p_long) {
		printw("%s %s %s %-4d%2s %s ",
				smode, owner, group,
				dir_entry->size, size, time);
		free(smode);
	}
#if ICONS
	if (show_icons) printw("%s ", icon);
#endif
	attron(cp);
	printw("%s", dir_entry->name);
	attroff(cp);
	printw("%c %s", f_ident, u_text);
	addch('\n');
}


char * prompt(char * t, char ** args) {
	int sub_cols = 30;
	int sub_rows = sub_cols / 2;
	// newwin(rows, cols, y, x)
	WINDOW * w = newwin(sub_rows, sub_cols, LINES / 2 - sub_rows / 2, COLS / 2 - sub_cols / 2);
	werase(w);
	
	box(w, 0, 0);

	// print text strinf
	size_t tlen = strlen(t);
	if (tlen < (unsigned)sub_cols - 2)
		mvwprintw(w, 2, sub_cols / 2 - tlen / 2, "%s", t);
	else {
		int row = 1;
		int col = 1;
		for (size_t i = 0; i < tlen; i++) {
			col++;
			if (i % (sub_cols - 4) == 0) {
				row++;
				col = 2;
			}
			mvwaddch(w, row, col, t[i]);
		}
	}

	if (!args) {
		wrefresh(w);
		napms(3500);
		delwin(w);
		return NULL;
	}

	size_t argcount = 0;
	size_t argstrlen = 0;
	for (char ** p = args; *p; p++) {
		argcount++;
		argstrlen += strlen(*p);
	}

	int cursor = 1;

	// pick option
	while (true) {
		int col = 2;
		for (size_t i = 0; i < argcount; i++) {
			if ((unsigned)cursor - 1 == i)
				wattron(w, COLOR_PAIR(WHITE) | A_REVERSE);
			mvwprintw(w, sub_rows - 2, col + (sub_cols / 2 - argstrlen), "%s", args[i]);
			if ((unsigned)cursor - 1 == i)
				wattroff(w, COLOR_PAIR(WHITE) | A_REVERSE);
			col += strlen(args[i]) + 2;
		}

		wrefresh(w);

		int c = getch();
		switch (c) {
			case KEY_UP:
			case KEY_LEFT:
			case CTRL_P:
			case CTRL_B:
			case 'h':
			case 'k':
				if (cursor > 1) cursor--;
				break;
			case KEY_DOWN:
			case KEY_RIGHT:
			case CTRL_N:
			case CTRL_F:
			case 'l':
			case 'j':
				if ((unsigned)cursor < argcount) cursor++;
				break;
			case '\n':
			case ' ':
				delwin(w);
				return args[cursor - 1];
			case 'q':
			case 27:
				goto done;
			default:
				break;
		}
	}
done:;
	 delwin(w);
	 return NULL;
}


char * curses_getline(char * p) {
	curs_set(1);
	echo();
	noraw();

	if (p)
		printw("%s", p);
	refresh();

	char * inp = malloc(128);
	size_t l = 0;
	char c;
	while ((c = getch()) != '\n') {
		if (c == 127) {
			if (l > 0) l--;
			addch(' ');
		}
		inp[l++] = c;
	}
	inp[l] = '\0';

	curs_set(0);
	noecho();
	raw();

	return inp;
}


void mark_all(void) {
	for (size_t i = 0; i < n_dir_entries; i++)
		dir_entries[i]->marked = true;
	n_marked_files = n_dir_entries;
}


void unmark_all(void) {
	for (size_t i = 0; i < n_dir_entries; i++)
		dir_entries[i]->marked = false;
	n_marked_files = 0;
}
