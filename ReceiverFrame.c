#include "ReceiverFrame.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>


frame * newFrame()
{
	frame * vec;

	vec = (frame*)malloc(sizeof(frame));

	if(vec == NULL)
		return NULL;

	vec->length = 0;
	vec->size = 0;
	vec->elements = NULL;

	return vec;
}

void delete(frame* vec)
{

	if(vec == NULL)
		return;


	/* liberta memoria do array de elements */
	free(vec->elements);

	/* liberta memoria da estrutura vetor */
	free(vec);
}

int assign(frame* vec, int pos, unsigned char val)
{

	if (vec == NULL || pos < 0 || pos >= vec->length)
		return -1;

	/* copia valor */
	vec->elements[pos].value = val;

	return pos;
}

int length(frame* vec)
{
	if(vec == NULL)
		return -1;

	return vec->length;
}

unsigned char elementAt(frame* vec, int indice)
{
	if (vec == NULL || indice < 0 || indice >= vec->length)
		return 0;

	return vec->elements[indice].value;
}

int insert(frame* vec, unsigned char valor, int pos) {
	int i;

	if(vec == NULL || pos < -1 || pos > vec->length) {
    return -1;
  }

	if(pos == -1) {
    pos = vec->length;
  }

	if(vec->length == vec->size) {
		if(vec->size == 0) {
      vec->size = 1;
    }
		else {
      vec->size *= 2;
    }

		vec->elements = (frame_element*)realloc(vec->elements, vec->size * sizeof(frame_element));

    if(vec->elements == NULL)
			return -1;
	}

	for(i=vec->length-1; i>=pos; i--)	{
		vec->elements[i+1].value = vec->elements[i].value;
	}

	vec->elements[pos].value = valor;

	vec->length++;

	return pos;
}

int removeElement(frame* vec, int pos)
{
	int i;

	if(vec == NULL || pos < 0 || pos >= vec->length)
		return -1;

	/* copia todos os elements a partir da posicao pos+1 ate ao fim do vetor para pos */
	for(i=pos+1; i<vec->length; i++)
	{
		vec->elements[i-1].value = vec->elements[i].value;
	}

	vec->length--;

	return 0;
}
