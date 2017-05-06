#include "life3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>

//Generate a key for the given triplet. Collisions can occour.
unsigned long gen_key(int x, int y, int z)
{   
    unsigned long hash = (x*677 + y)*971 + z;
    
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    
    return hash;
}

//Compute modulus
int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

//Generate index for insertion in the hashtable
int hashcode(int key, int SIZE_TABLE)
{
        return mod(key, SIZE_TABLE);
}

//Insert a live cell in the hashtable
void insert(cell * table, int x, int y, int z, int SIZE_TABLE)
{
        unsigned long key = gen_key(x, y, z);
        int hash = hashcode(key, SIZE_TABLE);
        cell new_cell =  {x, y, z, 1, NULL};
        
        
        if (table[hash].key == -1)
        {
                table[hash] = new_cell;
        }
        else
        {
                new_cell.next = malloc(sizeof(cell));
                *new_cell.next = table[hash];
                table[hash] = new_cell;
        }
        
}

//This function search in the hashtable the given cell and give back the cell status (0 or 1)
int search(cell * table, int x, int y, int z, int SIZE_TABLE)
{
        unsigned long key = gen_key(x,y,z);
        int hash = hashcode(key, SIZE_TABLE);
        
        if (table[hash].key == -1)
        {
                return 0;
        }
        else
        {
                cell current = table[hash];
                while (current.next != NULL && (current.x != x || current.y != y || current.z != z)){
                        current = *current.next;
                }
                
                if (current.x == x && current.y == y && current.z == z)
                        return 1;
                else
                        return 0;
                
        
        }
}


//TAKES EACH SET OF 3 COORDINATES AND INSERTS IN THE TABLE. STOPS WHEN IT FINDS A -1 WHICH SIGNALS THE END OF THE ARRAY
void formatarray(cell * table_B, int table[], int SIZE_TABLE){
	int i=0;
	while (table[i]>-1){
		insert(table_B, table[i], table[i+1], table[i+2], SIZE_TABLE);
		i=i+3;
	}
}

void insert_first_batch(cell * cur_table, cell* cur_table_low, cell* cur_table_high, int buffer[], int count, int SIZE_MAIN_TABLE, int SIZE_SIDE_TABLE, int lowB, int highB){
    int i=0;
    printf("Count: %d\n", count);
    while (i < count-1){
        if (buffer[i] == lowB)
            insert(cur_table_low, buffer[i], buffer[i+1], buffer[i+2], SIZE_SIDE_TABLE);
        else if (buffer[i] == highB)
            insert(cur_table_high, buffer[i], buffer[i+1], buffer[i+2], SIZE_SIDE_TABLE);
        else
            insert(cur_table, buffer[i], buffer[i+1], buffer[i+2], SIZE_MAIN_TABLE);

        i = i+3;
    }
}


//This function count the number of alive neighbours of a given dead cell

int num_alive_neighbours_d(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE)
{
    int num_live = 0;
    int rank;
    int find = search(aux_table, x, y, z, SIZE_TABLE);      

    if(!find) {   
		//THE ONLY CHANGE NECESSARY IS IN THE X COORDINATES. WE CHECK IF THE NEIGHBOURS WE WANT TO SEARCH ARE OUTSIDE THE BOUNDRIES OF
		//table, IF THEY ARE WE SEARCH IN lowerB TABLE (ARRAY WITH ALL THE NODES IN THE LOWER BOUND) OR IN THE TABLE higherB (HIGHER BOUNDRY)
	    if(mod((x+1),SIZE_CUBE)==highB) num_live += search(higherB, mod((x+1),SIZE_CUBE), y, z, SIZE_B_TABLE);
	    else num_live += search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE);

	    if(mod((x-1),SIZE_CUBE)==lowB) num_live += search(lowerB, mod((x-1),SIZE_CUBE), y, z, SIZE_B_TABLE);
	    else num_live += search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE);

	    num_live += search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE);
	    num_live += search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE);
	    num_live += search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE);
	    num_live += search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE);

	    if(num_live==2 || num_live==3){
		    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
		    insert(aux_table, x, y, z, SIZE_TABLE); 
		    //printf("%d: %d %d %d\n", rank, x, y, z);
		}
    }

    return num_live;
}


//This function count the number of alive neighbours of a given alive cell
int num_alive_neighbours_a(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE)
{
    int num_live = 0;
  
	//THE ONLY CHANGE NECESSARY IS IN THE X COORDINATES
    if(mod((x+1),SIZE_CUBE)!=highB){
    	if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE)) 
    	    num_alive_neighbours_d(table, aux_table, mod((x+1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE);
    	else num_live++;
    }else{
    	if(search(higherB, mod((x+1),SIZE_CUBE), y, z, SIZE_B_TABLE))  num_live++;
    }
    
    if(mod((x-1),SIZE_CUBE)!=lowB){
		if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) 
  	      num_alive_neighbours_d(table, aux_table, mod((x-1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE);
    	else num_live++;
    } else{
    	if(search(lowerB, mod((x-1),SIZE_CUBE), y, z, SIZE_B_TABLE)) num_live++;
    }

    //SAME    
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, mod((y+1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE);
    else num_live++;
    
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, mod((y-1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE);
    else num_live++;
    
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, y, mod((z+1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE);
    else num_live++;
    
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, y, mod((z-1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE);
    else num_live++;

    return num_live;
}

// This function frees the memory in the hashtable and set everything to -1 again
void free_list(cell * table, int SIZE_TABLE)
{       

    for(int temp=0; temp<SIZE_TABLE; temp++){
        
        cell test;
        cell current;
        current = table[temp];
	    if(current.key!=-1){
		    while (current.next != NULL) {
		        test=current;
		        current=*current.next;
		        free(test.next);
		    }   
        }
    }
    
    memset(table, -1, SIZE_TABLE*sizeof(cell));
}


//Scan the hash table and insert the cells, in order, in a linked list
void ordered_list(cell * table, cell ** order,  int SIZE_TABLE, int SIZE_CUBE)
{   
  
    for(int temp=0; temp<SIZE_TABLE; temp++){
        
        cell * current = &table[temp];
        
	    if(current->key != -1){	                
		    while (current->next != NULL) {
		  
		
		      if (order[current->x] == NULL){
		        cell * new_head = malloc(sizeof(cell));
		        new_head->next = NULL;
		        new_head->x = current->x;
		        new_head->y = current->y;
		        new_head->z = current->z;
		        new_head->key = current->key;
	            order[current->x] = new_head;
	       
	                
	            current = current->next;        
	          } else {
		                cell * head = order[current->x];
                        head = push(head, current->x, current->y, current->z);
                        order[current->x] = head;
                        current = current->next;
              }
                      
                      
		    }
		
	       
		      if (order[current->x] == NULL){
		        cell * new_head = malloc(sizeof(cell));
		        new_head->next = NULL;
		        new_head->x = current->x;
		        new_head->y = current->y;
		        new_head->z = current->z;
		        new_head->key = current->key;
	            order[current->x] = new_head;
	                       
	          } else {
		                cell * head = order[current->x];
                        head = push(head, current->x, current->y, current->z);
                        order[current->x] = head;
              }
    	}
    }
    

}


//Accessory function to support the push of new elements in the ordered linked list
cell * push(cell * head, int x, int y, int z) {     
    cell * current = head;
    
    if ((x < current->x) || (x == current->x && y < current->y) || (x == current->x && y == current->y && z < current->z))
    {
        cell * new_head = malloc(sizeof(cell));
        new_head->next = head;
        new_head->x = x;
        new_head->y = y;
        new_head->z = z;
        new_head->key = 1;
        
        return new_head;
    }

    
    while (current->next != NULL && ((current->next->x < x) || (current->next->x == x && current->next->y < y) || (current->next->x == x && current->next->y == y && current->next->z < z))) {
        current = current->next;
    }
    
    if (x == current->x && y==current->y && z==current->z)
        return head;
    
        
    if (current->next == NULL)
    {
        current->next = malloc(sizeof(cell));
        current->next->x = x;
        current->next->y = y;
        current->next->z = z;
        current->next->next = NULL;
    }
    else
    {
        cell * tmp = current->next;
        current->next = malloc(sizeof(cell));
        current->next->x = x;
        current->next->y = y;
        current->next->z = z;
        current->next->next = tmp;
    }
    
    return head;
}


//Print the cells of the given linked list
void print_list(cell ** order, int SIZE_CUBE)
{
     for (int i = 0; i < SIZE_CUBE; i++){
        if (order[i] != NULL){
                cell * current =  order[i];
                while (current->next != NULL)
                {
                        printf("%d %d %d\n", current->x, current->y, current->z);
                        current = current->next;
                }
        
        printf("%d %d %d\n", current->x, current->y, current->z);
        }
     }
}


//Accessory function: count the number of empty buckets in the hash table
int void_cells(cell * table, int SIZE_TABLE){
        int count = 0;
        for (int i = 0; i < SIZE_TABLE; i++){
                if (table[i].key == -1){
                        count ++;
                }
        }

        return count;
}




