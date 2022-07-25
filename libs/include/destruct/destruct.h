#ifndef DESTRUCT_H
#define DESTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Destruct
Destruct is a small ECS "addon" for the construct library!
*/

#include <construct/construct.h>

typedef unsigned int entity;

/* Returns the number of entities in the given entity buffer*/
unsigned int get_num_entities(entity** entities);
/* Returns a boolean value corresponding to the given entity having the specified component (Populates idx with the index of the corresponding element in specified buffer if idx is not NULL)*/
int has_component(entity ent, buffer component, unsigned int* idx);
/* Adds the specified component to the given entity */
void add_component(entity ent, buffer component);
/* Removes the specified component from a given entity (Gets ignored if entity doesn't have the specified component) */
void remove_component(entity ent, buffer component);
/* Returns a unique entity (Unique to the specified entity buffer) */
entity create_entity(entity** entities);
/* Removes the given entity from the specified entity buffer */
void destroy_entity(entity ent, entity** entities);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCT_H */