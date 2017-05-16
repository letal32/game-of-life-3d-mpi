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

        /*
        printf("Inserted: (%d, %d, %d)\n", new_cell.x, new_cell.y, new_cell.z);
        fflush(stdout);
        */
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
    //printf("Count: %d\n", count);
    while (i < count-1){
        //printf("Buffer: %d \n", buffer[i]);
        if (buffer[i] == lowB){
            insert(cur_table_low, buffer[i], buffer[i+1], buffer[i+2], SIZE_SIDE_TABLE);
            //printf("HERE1\n");
            //fflush(stdout);
        }
        else if (buffer[i] == highB){
            insert(cur_table_high, buffer[i], buffer[i+1], buffer[i+2], SIZE_SIDE_TABLE);
            //printf("HERE2\n");
            //fflush(stdout);
        }
        else {
            insert(cur_table, buffer[i], buffer[i+1], buffer[i+2], SIZE_MAIN_TABLE);
            //printf("HERE3\n");
            //fflush(stdout);
        }

        i = i+3;
    }
}


//This function count the number of alive neighbours of a given dead cell

int num_alive_neighbours_d(cell * table, cell * aux_table, cell * aux_low_table, cell * aux_high_table, int x, int y, int z, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE, int * num_down, int * num_up)
{
    int num_live = 0;
    int find = search(aux_table, x, y, z, SIZE_TABLE);      

    if(!find) {   
        num_live += search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE);
        num_live += search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE);
	    num_live += search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE);
	    num_live += search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE);
	    num_live += search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE);
	    num_live += search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE);

	    if((num_live==2 || num_live==3) && x>= mod(lowB+1, SIZE_CUBE) && x<=mod(highB-1, SIZE_CUBE)){
            //printf("Insertion point 3: (%d, %d, %d)\n", x, y, z);
		    insert(aux_table, x, y, z, SIZE_TABLE); 

            if (x == mod(lowB+1, SIZE_CUBE)){
              insert(aux_low_table, x, y, z, SIZE_B_TABLE);
              *num_down = *num_down + 1;
            }
            
            if (x == mod(highB-1, SIZE_CUBE)){
              insert(aux_high_table, x, y, z, SIZE_B_TABLE);
              *num_up = *num_up + 1;
            }

		}
    }

    return num_live;
}


//This function count the number of alive neighbours of a given alive cell
int num_alive_neighbours_a(cell * table, cell * aux_table, cell * aux_low_table, cell * aux_high_table, int x, int y, int z, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE, int * num_down, int * num_up)
{
    int num_live = 0;
  
	//THE ONLY CHANGE NECESSARY IS IN THE X COORDINATES
    if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, aux_low_table, aux_high_table, mod((x+1), SIZE_CUBE),y, z, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;
    
    if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, aux_low_table, aux_high_table, mod((x-1), SIZE_CUBE),y, z, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;

    //SAME    
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, aux_low_table, aux_high_table, x, mod((y+1),SIZE_CUBE), z, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;
    
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, aux_low_table, aux_high_table, x, mod((y-1),SIZE_CUBE), z, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;
    
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, aux_low_table, aux_high_table, x, y, mod((z+1),SIZE_CUBE), lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;
    
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, aux_low_table, aux_high_table, x, y, mod((z-1),SIZE_CUBE), lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;

    return num_live;
}

// This function frees the memory in the hashtable and set everything to -1 again
void free_list(cell * table, int SIZE_TABLE)
{       
    /*
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
    */
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
void print_list(cell ** order, int SIZE_CUBE, char * buffer)
{
     for (int i = 0; i < SIZE_CUBE; i++){
        if (order[i] != NULL){
                cell * current =  order[i];
                while (current->next != NULL)
                {
                        sprintf(buffer + strlen(buffer), "%d %d %d\n",current->x, current->y, current->z);
                        //fflush(stdout);
                        current = current->next;
                }
        
        sprintf(buffer + strlen(buffer), "%d %d %d\n",current->x, current->y, current->z);
        //fflush(stdout);
        }
     }
}


void print_table(cell * table, int SIZE_TABLE, char*buffer){

    cell * order[5];
    for (int l = 0; l<5; l++)
              order[l] = NULL;
   
    ordered_list(table, order, SIZE_TABLE, 5);
    print_list(order, 5, buffer);

    /*
    int i;
    int k = 0;
    for (i = 0; i < SIZE_TABLE; i++){
        if (table[i].key == -1)
            continue;

        cell cur_cell = table[i];

        while (cur_cell.next != NULL){
            printf("%d : (%d, %d, %d)\n", k,cur_cell.x, cur_cell.y, cur_cell.z);
            fflush(stdout);
            cur_cell = *cur_cell.next;
            k++;
        }

        if (cur_cell.next == NULL){
            printf("%d : (%d, %d, %d)\n", k,cur_cell.x, cur_cell.y, cur_cell.z);
            fflush(stdout);
            k++;
        }


    }
    */
}

void serialize(cell * table, int SIZE_TABLE, int dest[], int size){

    int i;
    int k = 0;

    for (i = 0; i < SIZE_TABLE; i++){
        if (table[i].key == -1)
            continue;

        cell cur_cell = table[i];

        while (cur_cell.next != NULL){
            dest[k] = cur_cell.x;
            dest[k+1] = cur_cell.y;
            dest[k+2] = cur_cell.z;
            k = k+3;

            cur_cell = *cur_cell.next; 
        }

        if (cur_cell.next == NULL){
            dest[k] = cur_cell.x;
            dest[k+1] = cur_cell.y;
            dest[k+2] = cur_cell.z;
            k = k+3;
        }


    }

}

void insert_table(cell * table, int SIZE_TABLE, int * buffer, int size){

    int i;
    for(i = 0; i < size; i=i+3){
            insert(table, buffer[i], buffer[i+1], buffer[i+2], SIZE_TABLE);
    }

}

/*
int num_alive_neighbours_dh(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE)
{
    int num_live = 0;
    int find = search(aux_table, x, y, z, SIZE_TABLE);      

    if(!find) {   
        //THE ONLY CHANGE NECESSARY IS IN THE X COORDINATES. WE CHECK IF THE NEIGHBOURS WE WANT TO SEARCH ARE OUTSIDE THE BOUNDRIES OF
        //table, IF THEY ARE WE SEARCH IN lowerB TABLE (ARRAY WITH ALL THE NODES IN THE LOWER BOUND) OR IN THE TABLE higherB (HIGHER BOUNDRY)
        if(mod((x+1),SIZE_CUBE)==highB) num_live += search(higherB, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE);
        else num_live += search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE);

        if(mod((x-1),SIZE_CUBE)==lowB) num_live += search(lowerB, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE);
        else num_live += search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE);

        num_live += search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE);
        num_live += search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE);
        num_live += search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE);
        num_live += search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE);

        if(num_live==2 || num_live==3){
            insert(aux_table, x, y, z, SIZE_TABLE); 
        }
    }

    return num_live;
}


//This function count the number of alive neighbours of a given alive cell
int num_alive_neighbours_ah(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE)
{
    int num_live = 0;
  
    //THE ONLY CHANGE NECESSARY IS IN THE X COORDINATES
    if(mod((x+1),SIZE_CUBE)!=highB){
        if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE)) 
            num_alive_neighbours_dh(table, aux_table, mod((x+1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE);
        else num_live++;
    }else{
        if(search(higherB, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE))  num_live++;
    }
    
    if(mod((x-1),SIZE_CUBE)!=lowB){
        if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) 
          num_alive_neighbours_dh(table, aux_table, mod((x-1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE);
        else num_live++;
    } else{
        if(search(lowerB, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) num_live++;
    }

    //SAME    
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table,  x, mod((y+1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE);
    else num_live++;
    
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table,  x, mod((y-1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE);
    else num_live++;
    
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table,  x, y, mod((z+1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE);
    else num_live++;
    
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table,  x, y, mod((z-1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE);
    else num_live++;

    return num_live;
}
*/

int num_alive_neighbours_ah(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_high)
{
    int num_live = 0;
  
    if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE, SIZE_CUBE, bound_high);
    else num_live++;
    
    if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE, SIZE_CUBE, bound_high);
    else num_live++;
    
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE, SIZE_CUBE, bound_high);
    else num_live++;
    
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE, SIZE_CUBE, bound_high);
    else num_live++;
    
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE, SIZE_CUBE, bound_high);
    else num_live++;
    
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_dh(table, aux_table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE, SIZE_CUBE, bound_high);
    else num_live++;

    return num_live;
}

int num_alive_neighbours_dh(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_high)
{
    int num_live = 0;
    if(!search(aux_table, x, y, z, SIZE_TABLE)) {   
        num_live += search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE);
        num_live += search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE);
        num_live += search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE);
        num_live += search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE);
        num_live += search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE);
        num_live += search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE);

        if((num_live==2 || num_live==3) && x == mod(bound_high+1, SIZE_CUBE)){
        insert(aux_table, x, y, z, SIZE_TABLE); 

        }
    }

    return num_live;
}

int num_alive_neighbours_al(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_low)
{
    int num_live = 0;
  
    if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_dl(table, aux_table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE, SIZE_CUBE, bound_low);
    else num_live++;
    
    if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_dl(table, aux_table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE, SIZE_CUBE, bound_low);
    else num_live++;
    
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_dl(table, aux_table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE, SIZE_CUBE, bound_low);
    else num_live++;
    
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_dl(table, aux_table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE, SIZE_CUBE, bound_low);
    else num_live++;
    
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_dl(table, aux_table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE, SIZE_CUBE, bound_low);
    else num_live++;
    
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_dl(table, aux_table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE, SIZE_CUBE, bound_low);
    else num_live++;

    return num_live;
}

int num_alive_neighbours_dl(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_low)
{
    int num_live = 0;
    if(!search(aux_table, x, y, z, SIZE_TABLE)) {   
        num_live += search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE);
        num_live += search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE);
        num_live += search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE);
        num_live += search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE);
        num_live += search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE);
        num_live += search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE);

        if((num_live==2 || num_live==3) && x == mod(bound_low-1, SIZE_CUBE)){
        insert(aux_table, x, y, z, SIZE_TABLE); 

        }
    }

    return num_live;
}





int num_alive_neighbours_d2(cell * table, cell * aux_table, cell * aux_low_table, cell * aux_high_table, cell* lower_row_table, cell* upper_row_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE, int * num_down, int * num_up)
{
    int num_live = 0;
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
            //printf("Insertion point 3: (%d, %d)\n", lowB+1, highB-1);
            insert(aux_table, x, y, z, SIZE_TABLE); 

            if (x == mod(lowB+1, SIZE_CUBE) || x == mod(lowB+2, SIZE_CUBE)){
              insert(aux_low_table, x, y, z, SIZE_B_TABLE);
              *num_down = *num_down + 1;
            }
            
            if (x == mod(highB-1, SIZE_CUBE) || x == mod(highB-2, SIZE_CUBE)){
              insert(aux_high_table, x, y, z, SIZE_B_TABLE);
              *num_up = *num_up + 1;
            }

            if (x == mod(lowB+1, SIZE_CUBE)){
                insert(lower_row_table, x, y, z, SIZE_B_TABLE);
            }

            if (x == mod(highB-1, SIZE_CUBE)){
                insert(upper_row_table, x, y, z, SIZE_B_TABLE);
            }

        }
    }

    return num_live;
}


//This function count the number of alive neighbours of a given alive cell
int num_alive_neighbours_a2(cell * table, cell * aux_table, cell * aux_low_table, cell * aux_high_table, cell* lower_row_table, cell* upper_row_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE, int * num_down, int * num_up)
{
    int num_live = 0;
  
    //THE ONLY CHANGE NECESSARY IS IN THE X COORDINATES
    if(mod((x+1),SIZE_CUBE)!=highB){
        if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE)) 
            num_alive_neighbours_d2(table, aux_table, aux_low_table, aux_high_table, lower_row_table, upper_row_table, mod((x+1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
        else num_live++;
    }else{
        if(search(higherB, mod((x+1),SIZE_CUBE), y, z, SIZE_B_TABLE))  num_live++;
    }
    
    if(mod((x-1),SIZE_CUBE)!=lowB){
        if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) 
          num_alive_neighbours_d2(table, aux_table, aux_low_table, aux_high_table, lower_row_table, upper_row_table,mod((x-1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
        else num_live++;
    } else{
        if(search(lowerB, mod((x-1),SIZE_CUBE), y, z, SIZE_B_TABLE)) num_live++;
    }

    //SAME    
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d2(table, aux_table, aux_low_table, aux_high_table,lower_row_table, upper_row_table, x, mod((y+1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;
    
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d2(table, aux_table, aux_low_table, aux_high_table,lower_row_table, upper_row_table, x, mod((y-1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;
    
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d2(table, aux_table, aux_low_table, aux_high_table,lower_row_table, upper_row_table, x, y, mod((z+1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;
    
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d2(table, aux_table, aux_low_table, aux_high_table,lower_row_table, upper_row_table, x, y, mod((z-1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_CUBE, SIZE_B_TABLE, num_down, num_up);
    else num_live++;

    return num_live;
}

int num_alive_neighbours_df(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_B_TABLE, int SIZE_CUBE) 
{ 
    int num_live = 0; 
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
            insert(aux_table, x, y, z, SIZE_TABLE);  
        } 
    } 
 
    return num_live; 
} 
 
 
//This function count the number of alive neighbours of a given alive cell 
int num_alive_neighbours_af(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_B_TABLE, int SIZE_CUBE) 
{ 
    int num_live = 0; 
   
    //THE ONLY CHANGE NECESSARY IS IN THE X COORDINATES 
    if(mod((x+1),SIZE_CUBE)!=highB){ 
        if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE))  
            num_alive_neighbours_df(table, aux_table, mod((x+1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_B_TABLE, SIZE_CUBE); 
        else num_live++; 
    }else{ 
        if(search(higherB, mod((x+1),SIZE_CUBE), y, z, SIZE_B_TABLE))  num_live++; 
    } 
     
    if(mod((x-1),SIZE_CUBE)!=lowB){ 
        if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE))  
          num_alive_neighbours_df(table, aux_table, mod((x-1),SIZE_CUBE), y, z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_B_TABLE, SIZE_CUBE); 
        else num_live++; 
    } else{ 
        if(search(lowerB, mod((x-1),SIZE_CUBE), y, z, SIZE_B_TABLE)) num_live++; 
    } 
 
    //SAME     
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE))  
        num_alive_neighbours_df(table, aux_table,  x, mod((y+1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_B_TABLE, SIZE_CUBE); 
    else num_live++; 
     
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE))  
        num_alive_neighbours_df(table, aux_table,  x, mod((y-1),SIZE_CUBE), z, lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_B_TABLE, SIZE_CUBE); 
    else num_live++; 
     
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE))  
        num_alive_neighbours_df(table, aux_table,  x, y, mod((z+1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_B_TABLE, SIZE_CUBE); 
    else num_live++; 
     
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE))  
        num_alive_neighbours_df(table, aux_table,  x, y, mod((z-1),SIZE_CUBE), lowerB, higherB, lowB, highB, SIZE_TABLE, SIZE_B_TABLE, SIZE_CUBE); 
    else num_live++; 
 
    return num_live; 
} 





