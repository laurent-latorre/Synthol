
// PoMAD Synthol Synthetizer
//
// Laurent Latorre - Polytech Montpellier - 2013
//
// Microelectronics & Robotics Dpt. (MEA)
// http://www.polytech.univ-montp2.fr/MEA
//
// Embedded Systems Dpt. (SE)
// http://www.polytech.univ-montp2.fr/SE

#include "stm32f4xx.h"
#include "llist.h"

#include <malloc.h>
#include <stdio.h>


// Add a note at the beginning of the list
// -------------------------------------------

llist add_note_first(llist list, uint8_t midi_note, uint8_t velocity)
{
    // Add a new element in memory

    note* new_note = malloc(sizeof(note));

    // Set the new element values

    new_note->midi_note = midi_note;
    new_note->velocity = velocity;

    // Set pointer to the previous first element

    new_note->nxt = list;

    // return the pointer to the new first element

    return new_note;
}


// Add a note at the end of the list
// ---------------------------------

llist add_note_last(llist list, uint8_t midi_note, uint8_t velocity)
{
    // Add a new element in memory

    note* new_note = malloc(sizeof(note));

    // Set the new element values

    new_note->midi_note = midi_note;
    new_note->velocity = velocity;

    // There is no next element

    new_note->nxt = NULL;

    // If list is empty, then simply return the newly created element

    if(list == NULL)
    {
        return new_note;
    }

    // Else, walk through the list to find the actual last element

    else
    {
    	note* temp=list;
        while(temp->nxt != NULL)
        {
            temp = temp->nxt;
        }
        temp->nxt = new_note;
        return list;
    }
}

// Delete a note based on the midi_note
// ------------------------------------

llist delete_note(llist list, uint8_t midi_note)
{
    // If list is empty, then just returns

    if(list == NULL)
        return NULL;

    // If the current element is the one to delete

    if(list->midi_note == midi_note)
    {
        note* tmp = list->nxt;
        free(list);
        tmp = delete_note(tmp, midi_note);
        return tmp;
    }

    // Else, the current element is not the one to delete

    else
    {
        list->nxt = delete_note(list->nxt, midi_note);
        return list;
    }
}


// Get pointer to last note in the list
// ------------------------------------

note* get_last_note(llist list)
{

    if(list == NULL)
    {
        return NULL;
    }

    else
        {
        	note* temp=list;
            while(temp->nxt != NULL)
            {
                temp = temp->nxt;
            }
            return temp;
        }
}


// Print the whole list (for debug only)
// -------------------------------------

void print_all_note(llist list)
{
    note *tmp = list;

    printf("Notes in the list : ");

    while(tmp != NULL)
    {
        printf("%d ", tmp->midi_note);
        tmp = tmp->nxt;
    }
}
