#ifndef RECEIVERFRAME_H

typedef struct
{
  unsigned char value;
} frame_element;

typedef struct
{
	int length;

  int size;

	frame_element * elements;

} frame;


frame * newFrame();

void delete(frame *vec);

int length(frame *vec);

unsigned char elementAt(frame *vec, int pos);

int insert(frame* vec, unsigned char valor, int pos);

int removeElement(frame* vec, int pos);

int assign(frame* vec, int pos, unsigned char val);

#define RECEIVERFRAME_H
#endif
