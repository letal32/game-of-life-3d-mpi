#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "life3d.h"

int main(int argc, char *argv[]){
   
   int x, y, z;
   int p;
   int id;
   int SIZE_CUBE;
   int iter;
   double elapsed_time;
   int BUFFER_MAX = 500000;
   
   
   char line[100];
   char * points;
   
   /* Initialize MPI  */
   
   MPI_Init(&argc, &argv);
   MPI_Barrier(MPI_COMM_WORLD);
   elapsed_time = -MPI_Wtime();
   
   /* Get the rank of the process and the size of the world (total number of processes) */
   
   MPI_Comm_rank (MPI_COMM_WORLD, &id);
   MPI_Comm_size (MPI_COMM_WORLD, &p);
   
   
   /* Read the file (only process 0) with the initial cells distribution and try
    * to compute boundaries in order to guarantee a fair distribution of alive cells among processes 
    * (This may not be necessary since the procedure is deterministic and each process can do this autonomously)
    * (Consider using process 0 to send all the cells to the various processes)
    */
    
   int boundaries[2*p];
   int buffer_size[p];
   
   memset(&boundaries, -1, 2*p*sizeof(int));
      
   if (id == 0){

        if (argv[1] == NULL) {
             printf("Error: file pointer is null.");
             exit(1);
        } 
   
        if (argv[2] == NULL) {
            printf("Error: Number of Iterations not specified.");
            exit(1);
        }
            
       FILE* file = fopen (argv[1], "r");
       fgets(line, sizeof(line), file);
       SIZE_CUBE = atoi(line);
       iter=atoi(argv[2]);
       
       int count[SIZE_CUBE];
       memset(&count, 0, SIZE_CUBE*sizeof(int)); 
       
       int totcells = 0; // COUNT TOTAL NUMBER OF INITIAL LIVING CELLS
       
       while (fgets(line, sizeof(line), file)) {
            points = strtok(line, " ");
            x = atoi(points);
            points = strtok (NULL, " ");
            y= atoi(points);
            points = strtok (NULL, " ");
            z = atoi(points);
            count[x] = count[x] + 1;
            totcells++;
       } 
       
       fclose (file); 
       
       
       int cell_proc = totcells/p; // NUMBER OF CELLS PER PROCESS (IDEALLY)
       
       
       for (int i = 0; i < SIZE_CUBE; i++){
            
            printf("%d : %d\n", i, count[i]);
            fflush(stdout);  
       }
       
       //printf("-----\n %d \n", cell_proc);
       //fflush(stdout);

       
       int average = totcells/SIZE_CUBE; //NUMBER OF CELLS PER SLICE
       boundaries[0] = 0;
       double perc;
       
       if (p > SIZE_CUBE){
            printf("Too many processes");
            exit(1);
       }
       else if (SIZE_CUBE > 2*p && average >= 2*p)
            perc = 0.90;
       else
            perc = 0.65;
       
       int partial_sum = 0;
       int partial_total = 0;
       int partials[p];
       int j = 1;
       int k = 0;
       for (int i = 0; i < SIZE_CUBE; i++){
            partial_sum += count[i];
            
            if (j == 2*p-1 || i == SIZE_CUBE-1){
                boundaries[j] = SIZE_CUBE-1;
                break;
            }
            
            partial_total += count[i];
            
            if (partial_sum >= perc*cell_proc){
                boundaries[j] = i;
                boundaries[j+1] = i+1;
                j += 2;
                partials[k] = partial_sum;
                k++;
                partial_sum = 0;
            } 
       }
       
       /* Check if boundaries have been computed correctly */
       
       for (int i = 0; i < 2*p; i++){
            if (boundaries[i] == -1){
                printf("Boundaries computation failed!");
                fflush(stdout);
                exit(3);
            }
       }
       
       
       partials[p-1] = totcells-partial_total;
       
      // printf("------------ BUFFER SIZE -------------- \n");
       
       for (int i=0; i < p; i++){
       
            int g_cells_up = count[mod(boundaries[2*i+1]+1, SIZE_CUBE)] + count[mod(boundaries[2*i+1]+2, SIZE_CUBE)];
            int g_cells_down = count[mod(boundaries[2*i]-1, SIZE_CUBE)] + count[mod(boundaries[2*i]-2, SIZE_CUBE)];
            buffer_size[i] = 3*(partials[i] + g_cells_up + g_cells_down);
            
            
            printf("Cells up for %d: %d \n", i, g_cells_up);
            printf("Cells down for %d: %d \n", i, g_cells_down);
            printf("Size of buffer of %d: %d \n", i, 3*(partials[i] + g_cells_up + g_cells_down));
            fflush(stdout);
            
            
       }
       
       
       printf("-------------- BOUNDARIES ------------\n");
       
       for (int i = 0; i < 2*p; i++){
            
            printf("%d ", boundaries[i]);
            fflush(stdout);  
       }
       printf("\n");
       
       printf("--------------PARTIALS ---------------\n");
       
       for (int i = p-1; i >= 0; i--){
            
            printf("%d ", partials[i]);
            fflush(stdout);  
       }
       printf("\n");
       
       
      
   }
   
   /* Broadcast boundary array to all nodes along with the size of the cube */ 
   
   MPI_Bcast(boundaries, 2*p, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&SIZE_CUBE, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&iter, 1, MPI_INT, 0, MPI_COMM_WORLD);
   
   
   //These are the boundaries of the current process ---> USE THESE VARIABLES IN THE MAIN ALGORITHM WHEN NEEDED
   int bound_low = boundaries[2*id];
   int bound_high = boundaries[2*id+1];

   /* x coordinates of the upper and lower ghost rows (rows sent from neighbours) */
   int ghost_up = mod(bound_high+1, SIZE_CUBE);   
   int ghost_down = mod(bound_low-1, SIZE_CUBE);

   /*
   printf("Ghost up process %d: %d\n", id, ghost_up);
   printf("Ghost down process %d: %d\n", id, ghost_down);
  */
   
   /*
   printf("Process %d boundaries: [%d, %d]\n", id, bound_low, bound_high);
   fflush(stdout);
  */
   /*
   printf("Process %d SIZE_CUBE: %d\n", id, SIZE_CUBE);
   fflush(stdout);
    */
    
   /*Process 0 sends to all other processes the packet of cells they have to elaborate */
   int *buffer0;
   if (id == 0){
        buffer0 = malloc(sizeof(int)*buffer_size[0]);
        for (int i = p-1; i >= 0; i--){
            
            int buffer[buffer_size[i]];
            
            FILE *file = fopen (argv[1], "r");
            fgets(line, sizeof(line), file);
            
            int k = 0;
            int l = 0;
            while (fgets(line, sizeof(line), file)) {
                points = strtok(line, " ");
                x = atoi(points);
                points = strtok (NULL, " ");
                y= atoi(points);
                points = strtok (NULL, " ");
                z = atoi(points);
                
                if ((i != 0) && (x >= boundaries[2*i] && x <= boundaries[2*i+1]) || (x == mod(boundaries[2*i]-1, SIZE_CUBE)) || (x == mod(boundaries[2*i]-2, SIZE_CUBE)) || (x==mod(boundaries[2*i+1]+1, SIZE_CUBE)) || (x==mod(boundaries[2*i+1]+2, SIZE_CUBE))){
                    buffer[k] = x;
                    buffer[k+1] = y;
                    buffer[k+2] = z;
                    k += 3;
                }    

                if ((i == 0) && (x >= boundaries[2*i] && x <= boundaries[2*i+1]) || (x == mod(boundaries[2*i]-1, SIZE_CUBE)) || (x == mod(boundaries[2*i]-2, SIZE_CUBE)) || (x==mod(boundaries[2*i+1]+1, SIZE_CUBE)) || (x==mod(boundaries[2*i+1]+2, SIZE_CUBE))){
                    buffer0[l] = x;
                    buffer0[l+1] = y;
                    buffer0[l+2] = z;
                    l += 3;
                }
                
            } 
       
            fclose (file); 
            
            
            printf("------------ Process %d BUFFER -----------\n", i);
            
            for (int j = 0; j < buffer_size[i]-2; j=j+3){
                printf("(%d, %d, %d)\n", buffer[j], buffer[j+1], buffer[j+2]);
                fflush(stdout);
            }
            
            
            /* Send the array to processes */
            
            if (i != 0)
                MPI_Send(&buffer, buffer_size[i], MPI_INT, i, 0, MPI_COMM_WORLD);
            
            
        }

   }
   

   
   /* TODO: (Only if sending arrays doesn't work) Each process reads the file and retains only the values */
   
   /* Receive the first cell batch */

   int first_batch[BUFFER_MAX];
   MPI_Status status_batch;
   int count_batch;

   if (id != 0){
        MPI_Recv(&first_batch, BUFFER_MAX, MPI_INT, 0, 0, MPI_COMM_WORLD, &status_batch);
        MPI_Get_count(&status_batch, MPI_INT, &count_batch);
        //printf("Received %d\n", count_batch);
        /*
        if (id == 3){
        printf ("--------------PROCESS %d -----------\n", id);
        for (int j = 0; j < count_batch; j = j+3){
            printf("(%d, %d, %d)\n", first_batch[j], first_batch[j+1], first_batch[j+2]);
            fflush(stdout);
            
        }  
      }
      */


      //printf("Process %d is here\n", id );
   }

   
   /* Insert the cells in first_batch inside the right tables and compute the first generation */
   
  int SIZE_MAIN_TABLE = 103001;  //The main table has more entries
  int SIZE_SIDE_TABLE = 103001;   //The tables containing only ghost rows have less entries on average so the table can be smaller

  cell * cur_main_table = malloc(SIZE_MAIN_TABLE*sizeof(cell));
  memset(cur_main_table, -1, SIZE_MAIN_TABLE*sizeof(cell));

  cell * low_main_table = malloc(SIZE_SIDE_TABLE*sizeof(cell));
  memset(low_main_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * high_main_table = malloc(SIZE_SIDE_TABLE*sizeof(cell));
  memset(high_main_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * aux_main_table = malloc(SIZE_MAIN_TABLE*sizeof(cell));
  memset(aux_main_table, -1, SIZE_MAIN_TABLE*sizeof(cell));

  cell * aux_low_table = malloc(SIZE_SIDE_TABLE*sizeof(cell));
  memset(aux_low_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * aux_high_table = malloc(SIZE_SIDE_TABLE*sizeof(cell));
  memset(aux_high_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * additional_high_table = malloc(SIZE_SIDE_TABLE*sizeof(cell));
  memset(additional_high_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * additional_low_table = malloc(SIZE_SIDE_TABLE*sizeof(cell));
  memset(additional_low_table, -1, SIZE_SIDE_TABLE*sizeof(cell));


  if (id != 0)
    insert_first_batch(aux_main_table, low_main_table, high_main_table, first_batch, count_batch, SIZE_MAIN_TABLE, SIZE_SIDE_TABLE, ghost_down, ghost_up);
  else
    insert_first_batch(aux_main_table, low_main_table, high_main_table, buffer0, buffer_size[0], SIZE_MAIN_TABLE, SIZE_SIDE_TABLE, ghost_down, ghost_up);

  /*
  if (id  == 2){
    print_table(aux_main_table, SIZE_MAIN_TABLE);
  }*/
  
  int i;
  cell *aux;
  cell current;
  int neighbours;
  int proc_up = mod(id + 1, p);
  int proc_down = mod(id-1, p);

  //printf("Process %d: up --> %d, down --> %d\n", id, proc_up, proc_down);
  for (i=0; i<iter; i++){
   
    aux=cur_main_table;
    cur_main_table = aux_main_table;
    aux_main_table=aux;
  
    free_list(aux_main_table, SIZE_MAIN_TABLE); 
  
    int temp;
    int num_up = 0;
    int num_down = 0;

    for(temp=0; temp < SIZE_MAIN_TABLE; temp++){
      current = cur_main_table[temp];
      if(current.key!=-1){
        neighbours= num_alive_neighbours_a(cur_main_table, aux_main_table, aux_low_table, aux_high_table, current.x, current.y, current.z, low_main_table, high_main_table, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_CUBE, SIZE_SIDE_TABLE, &num_down, &num_up);
        if (neighbours>=2 && neighbours<=4){

          insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE);

          if (current.x == bound_low){
            insert(aux_low_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
            num_down = num_down + 1;
          }

          if (current.x == bound_high){
            insert(aux_high_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
            num_up = num_up + 1;
          }
        }

        while (current.next != NULL) {
          current=*current.next;
          neighbours= num_alive_neighbours_a(cur_main_table, aux_main_table, aux_low_table, aux_high_table, current.x, current.y, current.z, low_main_table, high_main_table, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_CUBE, SIZE_SIDE_TABLE, &num_down, &num_up);
          if (neighbours>=2 && neighbours<=4){

            insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE);

            if (current.x == bound_low){
              insert(aux_low_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
              num_down = num_down+1;
            }

            if (current.x == bound_high){
              insert(aux_high_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
              num_up = num_up+1;
            }
          }
        } 
      }
    }
    
    /*
    //printf("NumUP: %d, NumDOWN: %d\n", num_up, num_down );
    int * ser = serialize(aux_low_table, SIZE_SIDE_TABLE, num_down*3);
    int l = 0;
    for (int h = 0; h < num_down*3; h=h+3){
        printf("%d : (%d, %d, %d)\n", l, ser[h], ser[h+1], ser[h+2]);
        l++;
    }
    */

      /* SEND GHOST ROWS AND RETRIEVE GHOST ROWS */
      /*
      if (id == 0 && i == 0){
        print_table(cur_main_table, SIZE_MAIN_TABLE);
      }
      */

      /*   
      if (id == 0){
        printf("PROCESS 0");

        printf("-----GENERATION %d NGT -----\n", i );
        print_table(aux_main_table, SIZE_MAIN_TABLE);
        printf("-----GENERATION %d LMT------\n", i);
        print_table(low_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(high_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d LAT------\n", i);
        print_table(aux_low_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(aux_high_table, SIZE_SIDE_TABLE);
      }

      MPI_Barrier(MPI_COMM_WORLD);

      if (id == 1){
        printf("PROCESS 1");
        printf("-----GENERATION %d NGT -----\n", i );
        print_table(aux_main_table, SIZE_MAIN_TABLE);
        printf("-----GENERATION %d LMT------\n", i);
        print_table(low_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(high_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d LAT------\n", i);
        print_table(aux_low_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(aux_high_table, SIZE_SIDE_TABLE);
      }
      */
      
      MPI_Request request_up;
      int buffer_recv_up[BUFFER_MAX];
      int TAG_SDOWN_RUP = 0;

      MPI_Irecv(buffer_recv_up, BUFFER_MAX, MPI_INT, proc_up, TAG_SDOWN_RUP, MPI_COMM_WORLD, &request_up);

      MPI_Request request_down;
      int buffer_recv_down[BUFFER_MAX];
      int TAG_SUP_RDOWN = 1;

      MPI_Irecv(buffer_recv_down, BUFFER_MAX, MPI_INT, proc_down, TAG_SUP_RDOWN, MPI_COMM_WORLD, &request_down);

      int send_up[num_up*3];
      serialize(aux_high_table, SIZE_SIDE_TABLE, send_up, num_up*3);

      MPI_Send(send_up, num_up*3, MPI_INT, proc_up, TAG_SUP_RDOWN, MPI_COMM_WORLD);

      int send_down[num_down*3]; 
      serialize(aux_low_table, SIZE_SIDE_TABLE,send_down , num_down*3);


      MPI_Send(send_down, num_down*3, MPI_INT, proc_down, TAG_SDOWN_RUP, MPI_COMM_WORLD);


      MPI_Status status_up;
      MPI_Status status_down;

      MPI_Wait(&request_up, &status_up);
      MPI_Wait(&request_down, &status_down);

      int num_recv_up;
      int num_recv_down;

      MPI_Get_count(&status_up, MPI_INT, &num_recv_up);
      MPI_Get_count(&status_down, MPI_INT, &num_recv_down);

      free_list(high_main_table, SIZE_SIDE_TABLE);
      free_list(low_main_table, SIZE_SIDE_TABLE);
      insert_table(high_main_table, SIZE_SIDE_TABLE, buffer_recv_up, num_recv_up);
      insert_table(low_main_table, SIZE_SIDE_TABLE, buffer_recv_down, num_recv_down);
      free_list(aux_high_table, SIZE_SIDE_TABLE);
      free_list(aux_low_table, SIZE_SIDE_TABLE);
      
      
      /*
      if (id == 0){
        printf("PROCESS 0");

        printf("-----GENERATION %d NGT -----\n", i );
        print_table(aux_main_table, SIZE_MAIN_TABLE);
        printf("-----GENERATION %d LMT------\n", i);
        print_table(low_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(high_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d LAT------\n", i);
        print_table(aux_low_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(aux_high_table, SIZE_SIDE_TABLE);
      }

      MPI_Barrier(MPI_COMM_WORLD);

      if (id == 1){
        printf("PROCESS 1");
        printf("-----GENERATION %d NGT -----\n", i );
        print_table(aux_main_table, SIZE_MAIN_TABLE);
        printf("-----GENERATION %d LMT------\n", i);
        print_table(low_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(high_main_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d LAT------\n", i);
        print_table(aux_low_table, SIZE_SIDE_TABLE);
        printf("-----GENERATION %d HMT------\n", i);
        print_table(aux_high_table, SIZE_SIDE_TABLE);
      }
      */
      
      
      
      if (i == iter-1){
   
          cell * order[SIZE_CUBE];
          for (int l = 0; l<SIZE_CUBE; l++)
              order[l] = NULL;
   
          ordered_list(aux_main_table, order, SIZE_MAIN_TABLE, SIZE_CUBE);

          if (id == 0){
            print_list(order, SIZE_CUBE);
            int ready = 1;
            MPI_Send(&ready, 1, MPI_INT, proc_up, 9, MPI_COMM_WORLD);
          } else {
            int ready;
            MPI_Status stat;
            MPI_Recv(&ready, 1, MPI_INT, proc_down, 9, MPI_COMM_WORLD, &stat);
            print_list(order, SIZE_CUBE);

            if (proc_up != 0){
              int ready = 1;
              MPI_Send(&ready, 1, MPI_INT, proc_up, 9, MPI_COMM_WORLD);
            }
          }

      }
      
      
   }    


        /*
        printf("------- NEXT GEN TABLE -------\n");
        print_table(aux_main_table, SIZE_MAIN_TABLE);

        printf("------- GHOST ROWS SEND UP----\n");
        print_table(aux_high_table, SIZE_SIDE_TABLE);

        printf("------- GHOST ROWS SEND DOWN-- \n");
        print_table(aux_low_table, SIZE_SIDE_TABLE);
        */


   /* From this point on I perform the communication of ghost cells to adjacent processes and receive the ghost cells from adjacent processes */
   
   
   
   
   /* Next generation starts */
   
   
   
   /*
   int dim[1],period[1];
   dim[0] = p;
   period[0] = 1;
   
   MPI_Comm ring_comm;
   MPI_Cart_create(MPI_COMM_WORLD, 1, dim, period, 0, &ring_comm);

   int coords[] = {1}; 
   
 
   if (id == 0){
   for (int i = 0; i < p; i++){
        MPI_Cart_coords(ring_comm, i, 1, coords);
        printf("---------- %d --------------\n", coords[0]);
   }
   }
  
   
   if (id == 1){
       int number = 1;
       MPI_Send(&number, 1, MPI_INT, 3, 0, ring_comm);
   }
   
   if (id == 3){
      int number;
      MPI_Recv(&number, 1, MPI_INT, 1, 0, ring_comm, &status_batch);
      printf("Number: %d", number);
   }
   */  

   //elapsed_time += MPI_Wtime();

   //printf("%f\n", elapsed_time);
   MPI_Finalize();
}   
     
   
   
   
   
   
   
   
   
   
   
   

