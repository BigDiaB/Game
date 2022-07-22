#ifndef ECS_H
#define ECS_H

/*
buffer tag_component = init_bufferva(0,2,UINT,VOID);

unsigned int entity = create_entity();
add_component(entity,tag_component);

if (has_component(entity,tag_component,NULL))
    puts("Hello, World!");

remove_component(entity,tag_component);
destroy_entity(entity);

deinit_buffer(tag_component);
*/

unsigned int pseudo_random_premutation(unsigned int x)
{
    const unsigned int prime = 4294967291;
    if (x >= prime)
        return x;
    unsigned int residue = ((unsigned long long) x * x) % prime;
    return (x <= prime / 2) ? residue : prime - residue;
}

bool has_component(unsigned int entity, buffer component, unsigned int* idx)
{
    unsigned int i, size = get_buffer_length(component);

    for (i = 0; i < size; i++)
    {
        if (get_buffer_fieldui(component,i,0) == entity)
        {
            if (idx != NULL)
                *idx = i;
            return true;
        }
    }
    return false;
}

void add_component(unsigned int entity, buffer component)
{
    if (has_component(entity,component,NULL))
        return;
    resize_buffer(component,get_buffer_length(component)+1);
    set_buffer_fieldui(component,get_buffer_length(component)-1,0,entity);
}

void remove_component(unsigned int entity, buffer component)
{
    unsigned int index;
    if (!has_component(entity,component,&index))
        return;

    remove_buffer_at(component,index);
}

unsigned int create_entity(unsigned int** entities)
{
    if ((*entities) == NULL)
    {
        (*entities) = malloc(sizeof(unsigned int));
        (*entities)[0] = 0;
    }

    (*entities)[0]++;
    if ((*entities)[0] == 0)
    {
        puts("More than 2^32 entities! Internal buffer overflown!");
        exit(EXIT_FAILURE);
    }
    (*entities) = realloc((*entities),(*entities)[0]);

    return (*entities)[(*entities)[0]] = pseudo_random_premutation((*entities)[0]-1);
}

void destroy_entity(unsigned int entity, unsigned int** entities)
{
    unsigned int i;
    bool found = false;
    for (i = 1; i < (*entities)[0] +1; i++)
    {
        found = (*entities)[i] == entity;
        if (found)
            break;
    }

    if (found)
    {
        if (--(*entities)[0] == 0)
        {
            free((*entities));
            (*entities) = NULL;
            return;
        }

        for (; i < (*entities)[0]; i++)
        {
            (*entities)[i] = (*entities)[i + 1];
        }
    }
}

#endif /* ECS_H */