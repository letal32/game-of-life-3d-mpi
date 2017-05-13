#ifndef life3d
#define life3d

typedef struct node {
    int x;
    int y;
    int z;
    int key; 
    struct node * next;
} cell;

unsigned long gen_key(int x, int y, int z);
int mod(int a, int b);
int hashcode(int key, int SIZE_TABLE);
void insert(cell * table, int x, int y, int z, int SIZE_TABLE);
int search(cell * table, int x, int y, int z, int SIZE_TABLE);
void formatarray(cell * table_B, int table[], int SIZE_TABLE);
int num_alive_neighbours_d2(cell * table, cell * aux_table, cell * aux_low_table, cell * aux_high_table, cell* lower_row_table, cell* upper_row_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE, int * num_down, int * num_up);
int num_alive_neighbours_a2(cell * table, cell * aux_table, cell * aux_low_table, cell * aux_high_table, cell* lower_row_table, cell* upper_row_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_CUBE, int SIZE_B_TABLE, int * num_down, int * num_up);
void free_list(cell * table, int SIZE_TABLE);
void ordered_list(cell * table, cell ** order,  int SIZE_TABLE, int SIZE_CUBE);
cell * push(cell * head, int x, int y, int z);
void print_list(cell ** order, int SIZE_CUBE);
void insert_first_batch(cell * cur_table, cell* cur_table_low, cell* cur_table_high, int buffer[], int count, int SIZE_MAIN_TABLE, int SIZE_SIDE_TABLE, int lowB, int highB);
void print_table(cell * table, int SIZE_TABLE);
void serialize(cell * table, int SIZE_TABLE, int dest[], int size);
void insert_table(cell * table, int SIZE_TABLE, int * buffer, int size);
int num_alive_neighbours_dh(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_high);
int num_alive_neighbours_ah(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_high);
int num_alive_neighbours_al(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_low);
int num_alive_neighbours_dl(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, int bound_low);
int num_alive_neighbours_af(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_B_TABLE, int SIZE_CUBE);
int num_alive_neighbours_df(cell * table, cell * aux_table, int x, int y, int z, cell * lowerB, cell * higherB, int lowB, int highB, int SIZE_TABLE, int SIZE_B_TABLE, int SIZE_CUBE);


#endif