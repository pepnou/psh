#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>


#include "pash.h"



int width, height;
int over = 0;

void resize()
{
	struct winsize w;
	ioctl( 0, TIOCGWINSZ, &w);
	width = w.ws_col;
	height = w.ws_row;
}

//envoyer ctrl+d dans stdin (jsp comment faire)
void end()
{
	//fonctionne mais attend la prochaine entré dans stdin car read est bloquant
	over = 1;

	//pas mal mais ferme stdin --'
	/*int fd[2];
	pipe(fd);
	close(0); // 0:stdin
	dup2(fd[0], 0); // make read pipe be stdin
	close(fd[0]);
	fd[0] = 0;*/

	//write( STDIN_FILENO, "\4", 1);
}

size_t prompt()
{
	// http://ezprompt.net/
	char pr[] = "\nCC$ ";
	write(STDOUT_FILENO, pr, strlen(pr));

	return strlen(pr) - 1; // -1 pour enlever la longueur de \n
}

void moveC(size_t* source, size_t* dest, size_t* prw)
{
	int lS = (*source + *prw) / width;
	int lD = (*dest + *prw) / width;

	int ldiff = lD - lS;
	
	if(ldiff < 0)
	{
		for(int i = 0; i < -1*ldiff; i++)
			write(STDOUT_FILENO, UPC, strlen(UPC));
	}
	else if(ldiff > 0)
	{
		for(int i = 0; i < ldiff; i++)
			write(STDOUT_FILENO, DOWNC, strlen(DOWNC));
	}

	int cS = (*source + *prw) % width;
	int cD = (*dest + *prw) % width;

	int cdiff = cD - cS;

	if(cdiff < 0)
	{
		for(int i = 0; i < -1*cdiff; i++)
			write(STDOUT_FILENO, BACKC, strlen(BACKC));
	}
	else if(cdiff > 0)
	{
		for(int i = 0; i < cdiff; i++)
			write(STDOUT_FILENO, FORWC, strlen(FORWC));
	}
}

void eraseLine(size_t* cur, size_t* fin, size_t* prw)
{

	moveC(cur, fin, prw);

	int nbL = (*fin + *prw) / width;
	int nbC = (*prw + *fin) % width;
	
	for(int i = 0; i < nbC; i++)
		write(STDOUT_FILENO, BACKC, strlen(BACKC));

	for(int i = 0; i < nbL; i++)
	{
		write(STDOUT_FILENO, DELLI, strlen(DELLI));
		write(STDOUT_FILENO, UPC, strlen(UPC));
	}

	for(int i = 0; i < *prw; i++)
		write(STDOUT_FILENO, FORWC, strlen(FORWC));

	write(STDOUT_FILENO, DELLI, strlen(DELLI));
}

void autoComp(char* buf, size_t* cur, size_t* fin, size_t* size)
{

}

void handle( char c, char* buf, size_t* cur, size_t* fin, size_t* size, size_t* prw, historique* h)
{
	if(c >= 32 && c <= 126)
	{
		if(*cur == *fin)
		{
			write(STDOUT_FILENO, &c, 1);
			buf[*cur] = c;

			if((*cur + *prw + 1) % width == 0)
			{
				write(STDOUT_FILENO, " ", 1);
				write(STDOUT_FILENO, BACKC, strlen(BACKC));
			}
			(*cur)++;
			(*fin)++;
		}
		else
		{
			strncpy( &(buf[*cur + 1]), &(buf[*cur]), *fin - *cur + 1);
			buf[*cur] = c;
						
			write(STDOUT_FILENO, &(buf[*cur]), *fin - *cur + 1);
			
			if((*fin + *prw + 1) % width == 0)
			{
				write(STDOUT_FILENO, " ", 1);
				write(STDOUT_FILENO, BACKC, strlen(BACKC));
			}

			(*cur)++;
			(*fin)++;

			moveC(fin, cur, prw);
		}
	}
	else
	{
		switch(c)
		{
			//terminer le shell : ctrl + D
			case 4:
			{
				over = 1;
				write(STDOUT_FILENO, "\n", 1);
				break;
			}
			//new line : ctrl + J
			case 10:
			{
				h->cur = 0;
				if(*fin != 0)
					ajoutDeb(&(h->liste), buf, *fin + 1);

				*prw = prompt();

				(*cur) = (*fin) = 0;
				write(STDOUT_FILENO, SAVEC, strlen(SAVEC));
				break;
			}
			//effacer tout : ctrl + U
			case 21:
			{
				eraseLine(cur, fin, prw);
				(*cur) = (*fin) = 0;
				break;
			}
			//effacer un caractère
			case 127:
			{
				if(*cur == 0)
					break;

				strncpy( &(buf[*cur - 1]), &(buf[*cur]), *fin - *cur + 1);

				eraseLine( cur, fin, prw);

				write(STDOUT_FILENO, buf, *fin - 1);

				if((*fin + *prw - 1) % width == 0)
				{
					write(STDOUT_FILENO, " ", 1);
					write(STDOUT_FILENO, BACKC, strlen(BACKC));
				}

				(*cur)--;
				(*fin)--;

				moveC(fin, cur, prw);

				break;
			}
			//partie avec les flèches
			case 27:
			{
				read(STDIN_FILENO, &c, 1);

				if(c == '[')
				{
					read(STDIN_FILENO, &c, 1);

					switch(c)
					{
						case 'A':
						{
							elem* tmp = h->liste;
							int i;
							for(i = 0; i < h->cur; i++)
							{
								if(!tmp)
									break;
								tmp = tmp->suiv;
							}
							if(tmp)
							{
								eraseLine( cur, fin, prw);

								(h->cur)++;
								*cur = 0;
								*fin = tmp->size - 1;

								strncpy(buf, tmp->buf, tmp->size);
								write(STDOUT_FILENO, buf, tmp->size);

								moveC( fin, cur, prw);
							}
							break;
						}
						case 'B':
						{
							if(h->cur == 0)
								break;

							if(h->cur == 1)
							{
								eraseLine( cur, fin, prw);

								(h->cur)--;
								*cur = *fin = 0;

								break;
							}


							elem* tmp = h->liste;
							int i;
							for(i = 0; i < h->cur - 2; i++)
								tmp = tmp->suiv;

							eraseLine( cur, fin, prw);

							(h->cur)--;
							*cur = 0;
							*fin = tmp->size - 1;

							strncpy(buf, tmp->buf, tmp->size);
							write(STDOUT_FILENO, buf, tmp->size);

							moveC( fin, cur, prw);
							break;
						}
						case 'C':
						{
							if(*cur < *fin)
							{
								moveC( cur, fin, prw);
								(*cur)++;
								moveC( fin, cur, prw);
							}
							break;
						}
						case 'D':
						{
							if(*cur > 0)
							{
								moveC( cur, fin, prw);
								(*cur)--;
								moveC( fin, cur, prw);
							}
							break;
						}
						default:
						{
							handle(c, buf, cur, fin, size, prw, h);
							break;
						}
					}
				}
				else
				{
					handle(c, buf, cur, fin, size, prw, h);
				}

				break;
			}
			default:
			{
				printf("%d\n", c);
			}
		}
	}
}







int main( int argc, char** argv, char** envp)
{

	signal( SIGWINCH, &resize);
	signal( SIGINT, &end);

	static struct termios old, new1;
	int echo = 0;

	tcgetattr(0, &old); /* grab old terminal i/o settings */
    new1 = old; /* make new settings same as old settings */
    new1.c_lflag &= ~ICANON; /* disable buffered i/o */
    new1.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
    tcsetattr(0, TCSANOW, &new1); /* use these new terminal i/o settings now */
    


	char c, *buf;
	size_t cur, fin, size = 1024, prw;
	cur = fin = 0;
	if(!(buf = malloc(size)))
	{
		perror("allocation du buffer : ");
		exit(1);
	}
	
	resize();
	prw = prompt();
	write(STDOUT_FILENO, SAVEC, strlen(SAVEC));

	historique h;
	h.cur = 0;
	h.liste = NULL;

	do
	{
		read(STDIN_FILENO, &c, 1);

		handle(c, buf, &cur, &fin, &size, &prw, &h);

	} while(!over);



	tcsetattr(0, TCSANOW, &old);

	exit(0);
}
