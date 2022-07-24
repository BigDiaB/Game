#ifndef DESTRUCT_H
#define DESTRUCT_H

/* Destruct

Destruct is a small ECS "addon" for the construct library!
Usage example:
*/

#include <construct/construct.h>

#ifndef DESTRUCT_H	
	/* Create a "world", where all of your entities will live in (There can only be 2^32-1 entities maximum! There is an error message though) */
	unsigned int* entities = NULL;

	/* Initialise a component buffer (The first element needs to be an unsigned integer to hold the entity id! The rest are the actual types) */
	buffer tag_component = init_bufferva(0,2,UINT,VOID);

	unsigned int entity = create_entity(&entities);
	add_component(entity,tag_component);

	/* If you supply has_component() with an unsigned integer pointer instead of NULL it will fill in the index of the corresponding element from the buffer */
	if (has_component(entity,tag_component,NULL))
	    puts("Hello, World!");

	/* Clean up the entities and components */
	remove_component(entity,tag_component);
	destroy_entity(entity,&entities);

	deinit_buffer(tag_component);
#endif

int has_component(unsigned int entity, buffer component, unsigned int* idx);
void add_component(unsigned int entity, buffer component);
void remove_component(unsigned int entity, buffer component);
unsigned int create_entity(unsigned int** entities);
void destroy_entity(unsigned int entity, unsigned int** entities);

#endif /* DESTRUCT_H */