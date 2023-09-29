#include <iostream>
#include <curses.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>

#define COLOR_NORMAL 1
#define COLOR_SELECT 2
#define MAX_OPT_LEN 128

int main() {
    int input, HEIGHT, WIDTH, x_offset;
    const char options[][MAX_OPT_LEN] = {"Use default /bin/bash", "Use alternative :)", "Exit"};

    const int possibs = sizeof(options)/sizeof(options[0]);
    int selection = 0;

    initscr();
    if (!has_colors())
        return 1;
    clear();noecho();raw();curs_set(0);start_color();keypad(stdscr, true);

    getmaxyx(stdscr, HEIGHT, WIDTH);
    int y_offset = (HEIGHT-possibs)/2;

    init_pair(COLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_SELECT, COLOR_BLACK, COLOR_WHITE);
    attron(COLOR_PAIR(COLOR_NORMAL));

    while (true) 
    {
        selection %= possibs;
        for (int i=0; i<possibs; i++) 
        {
            x_offset = (WIDTH-strlen(options[i]))/2;
            if (i==selection)
                attron(COLOR_PAIR(COLOR_SELECT));
            mvprintw(i+y_offset, x_offset, options[i]);
            attron(COLOR_PAIR(COLOR_NORMAL));
        }
        input = getch();
        switch(input) 
        {
            case KEY_UP: 
                selection--;
                if (selection==-1) 
                    selection=possibs-1;
                break;
            case KEY_DOWN:
                selection++;
                break;
            case 10:
                if (selection == 0)
                {
                    endwin();
                    system("sudo -u user2 ash -c 'cd ~ && /bin/ash'");
                    exit(0);
                }
                else if (selection == 1)
                {
                    endwin();
                    system("sudo -u user2 ash -c 'cd /home/user2 && sudo ./hw'");
                    exit(0);
                }
                else if (selection == 2)
                {
                    endwin();
                    exit(0);
                }
        }
    }
    return 0;
}
