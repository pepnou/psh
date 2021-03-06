#include <string.h>
#include <stdlib.h>

#include "liste.h"


void ajoutDeb(elem** liste, char* buf, size_t size)
{
	elem * new_elem = malloc(sizeof(elem));
	if(new_elem == NULL)
	{
		perror("ajoutDeb");
		exit(1);
	}

	new_elem->size = size;
	new_elem->buf = malloc(size + 1);
	strncpy(new_elem->buf, buf, size);
	new_elem->buf[size] = '\0';
	new_elem->suiv = *liste;

	*liste = new_elem;
}

void supprList(elem* liste)
{
	elem* tmp;

	while(liste != NULL)
	{
		tmp = liste->suiv;

		free(liste->buf);
		free(liste);

		liste = tmp;
	}
}


void ajoutDeb2(elem2** liste, int val, char* buf)
{
	elem2* new_elem = malloc(sizeof(elem));
	if(new_elem == NULL)
	{
		perror("ajoutDeb");
		exit(1);
	}

	new_elem->val = val;
	new_elem->buf = malloc(strlen(buf) + 1);
	strcpy(new_elem->buf, buf);
	new_elem->suiv = *liste;

	*liste = new_elem;
}

void supprList2(elem2* liste)
{
	elem2* tmp;

	while(liste != NULL)
	{
		tmp = liste->suiv;
		free(liste->buf);
		free(liste);
		liste = tmp;
	}
}