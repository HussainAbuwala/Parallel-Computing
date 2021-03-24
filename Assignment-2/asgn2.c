/*

	NAME: HUSSAIN SADIQ ABUWALA
	STUDENT-ID: 27502333

*/


#include <stdio.h>
#include "mpi.h"
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#define BASE_STATION_RANK 0
#define total_base_station 1
#define total_nodes 20
#define n_rows 4
#define n_cols 5
#define total_MPI_PROC 21
#define total_iterations 200
#define iteration_length 1000
#define range_lower_bound 1
#define range_upper_bound 20
#define EVENT_TAG 0
#define EXIT_TAG 1

//struct datatype used to send it to base station
struct node {
		int iteration_number;
		int my_rank;
		int messages_sent;
		int event_nodes[4];
		int timeStamp;	
};

int find_row(int node_grid[n_rows][n_cols],int rank);
void fill_grid(int node_grid[n_rows][n_cols]);
int find_my_N(int rank,int neighbours[4],int node_grid[n_rows][n_cols]);
void print_N(int neighbours[4],int rank);
void fill_N_N_count(int myneighbours[4],int NNcount[4],int node_grid[n_rows][n_cols]);
int find_N_N(int neighbour_rank,int node_grid[n_rows][n_cols]);
void checkSleep(double start, double end);
void send_r_num_to_neighbours(int myneighbours[4],int NNcount[4],int *r_n,MPI_Request *r);
void receive_r_num_from_neighbours(int total_n,int r_nums[4],int myneighbours[4],MPI_Request *r,int snd);
int find_send_valid_N(int myneighbours[4],int NNcount[4]);
int find_recv_valid_N(int total_neighbours);
void printRandArr(int r_nums[4]);
void reset(int r_nums[4]);
int checkIfEvent(int r_nums[4],int recv_tot,int n_i[4]);
void prepare_Struct(int i,int ref_rank,int msg_sent,int nodes_inv[4],struct node *n,int nebo[4],int ts);
void printStruct(struct node n);
void createCustom_data_type(MPI_Datatype *mystruct);
void sendToBase_station(struct node *n,MPI_Datatype mystruct);
void send_exit_to_base(struct node *n,MPI_Datatype mystruct);
void baseStation_write_event_info(struct node base_recv,FILE * fp);
void baseStation_write_simulation_summary(int t_ev,int t_b, int t_e, int t_n,FILE *fp);

int main(int argc,char **argv){

	int node_grid[n_rows][n_cols];
	
	//index 0 - up neighbour rank
	//index 1 - down neighbour rank
	//index 2 - left neighbour rank
	//index 3 - right neighbour rank
	
	//if any position up/down/left/right has -1 stored, it means that node does not have
												//up/down/left/right direction neighbour

	int myneighbours[4] = {-1,-1,-1,-1};

	//index 0 - if(up neighbour exists), then number of neighbours up neighbour has
	//index 1 - if(down neighbour exists), then number of neighbours down neighbour has
	//index 2 - if(left neighbour exists), then number of neighbours left neighbour has
	//index 3 - if(right neighbour exists), then number of neighbours right neighbour has
	int NNcount[4] = {-1,-1,-1,-1};

	int r_nums[4] = {-1,-1,-1,-1};
	int	nodes_involved[4] = {-1,-1,-1,-1};

	//total neighbours a particular ranked node process has
	int total_neighbours;

	int send_val_N;
	int recv_val_N;

	int rank,size,iteration_counter;
	int random_number;
	
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	MPI_Datatype mystruct;

	//MPI custome datatype is created to support my struct type
	createCustom_data_type(&mystruct);
	
	if(rank != BASE_STATION_RANK){
		
		//pseudo random generator is initialised
		srand(time(NULL) | rank);

		//two-d grid is filled with rank values of nodes starting from 1
		fill_grid(node_grid);	

		//total neighbours are found out
		total_neighbours = find_my_N(rank,myneighbours,node_grid);
		fill_N_N_count(myneighbours,NNcount,node_grid);
		send_val_N = find_send_valid_N(myneighbours,NNcount);
		recv_val_N = find_recv_valid_N(total_neighbours);
		
		//printf("rank = %d, send_valid_n = %d, recv_valid_n = %d\n",rank,send_val_N,recv_val_N);
		for(iteration_counter = 0; iteration_counter < total_iterations; iteration_counter++ ){
				//start timer
			  double start = MPI_Wtime();
				reset(r_nums);
				reset(nodes_involved);
				struct node n;
				//start working
				MPI_Request requests[send_val_N + recv_val_N];
				MPI_Status	statuses[send_val_N + recv_val_N];
				
				//random number is generated
				random_number = (random() % (range_upper_bound+1-range_lower_bound))+range_lower_bound;
				//printf("rank = %d, i = %d, random_n = %d ",rank,iteration_counter,random_number);
				
				//send my random numbers to my neighbors
				send_r_num_to_neighbours(myneighbours,NNcount,&random_number,requests);

				//receive random numbers of my neighbors
				receive_r_num_from_neighbours(total_neighbours,r_nums,myneighbours,requests,send_val_N);

				//wait for messages to be sent and received
				MPI_Waitall(send_val_N + recv_val_N,requests,statuses);
				//printRandArr(r_nums);
				int result = checkIfEvent(r_nums,recv_val_N,nodes_involved);

				//check if event has happened
				if(result == 1){
						//printf("rank = %d, i = %d, random_n = %d ",rank,iteration_counter,random_number);
						//printRandArr(r_nums);
						int timestamp = time(NULL);
						//prepare struct to send to base station
						prepare_Struct(iteration_counter,
													 rank,
													 send_val_N,
													 nodes_involved,
													 &n,
													 myneighbours,timestamp);
					  //printStruct(n);
						//send struct to base station
						sendToBase_station(&n,mystruct);
				}
				//end working
				//end timer				
				double end = MPI_Wtime();

				//sleep if needed
				checkSleep(start,end);
		}
	struct node y;
	y.messages_sent = total_iterations * send_val_N;
	//send exit message to base station
	send_exit_to_base(&y,mystruct);
	}


	else if(rank == BASE_STATION_RANK){
		FILE * fp;
		fp = fopen("logfile.txt","w");
		
		fprintf(fp,"----------------------------- START OF SIMULATION--------------------------\n");
		fprintf(fp,"-----------------------------SIMULATION PARAMETERS ARE SHOWN BELOW---------\n");
		fprintf(fp,"BASE STATION RANK = %d\n",BASE_STATION_RANK);
		fprintf(fp,"TOTAL WSN NODES = %d\n", total_nodes);
		fprintf(fp,"TWO-D GRID DIMENSIONS ARE = %d ROWS, %d COLUMNS \n",n_rows,n_cols);
		fprintf(fp,"TOTAL MPI PROCESSES = %d\n",total_MPI_PROC);
		fprintf(fp,"TOTAL ITERATIONS = %d\n",total_iterations);
		fprintf(fp,"EACH ITERATION LENGTH = %d MILLISECONDS\n",iteration_length);
		fprintf(fp,"RANDOM NUMBER RANGE: %d <= RANDOM NUMBER <= %d\n",range_lower_bound,
																																	range_upper_bound);
		fprintf(fp,"---------------------------------------------------------------------------\n");
		fprintf(fp,"\n");

		int count = 0;
		int total_events = 0;
		int total_messages_to_base_event = 0;
		int total_exit_messages_to_base = total_nodes;
		int total_messages_between_nodes = 0;

		//keep looping until all nodes have not exited
		while(count != total_nodes){
				struct node base_recv;
				MPI_Status status;
		
				//receive message from node
				MPI_Recv(&base_recv,
								 1,
								 mystruct,
								 MPI_ANY_SOURCE,
								 MPI_ANY_TAG,
								 MPI_COMM_WORLD,&status);
				
				//check if message is event-related
				if(status.MPI_TAG == EVENT_TAG){
				
					total_events = total_events + 1;
					total_messages_to_base_event = total_messages_to_base_event + 1;
					baseStation_write_event_info(base_recv,fp);

				}
				//check if message is exit related
				else if (status.MPI_TAG == EXIT_TAG){
				 	count = count + 1;
					total_messages_between_nodes = total_messages_between_nodes + base_recv.messages_sent;
				} 				
				//printStruct(base);
				//if(base.iteration_number == total_iterations) count = count + 1;
				//printStruct(base);
		}
		//write to log file
		baseStation_write_simulation_summary(total_events,
																				 total_messages_to_base_event,
																 				 total_exit_messages_to_base,
																				 total_messages_between_nodes,fp);
		fclose(fp);	
	}
	
	//finish MPI environment
	MPI_Finalize();
	return 0;

	
}

//write to log file
void baseStation_write_simulation_summary(int t_ev,int t_b, int t_e, int t_n,FILE *fp){
	
	fprintf(fp,"\n");
	fprintf(fp,"\n");
	fprintf(fp,"\n");
	fprintf(fp,"\n");
	fprintf(fp,"----------------------------- SUMMARY --------------------------------------\n");
	fprintf(fp,"\n");
	fprintf(fp,"Total number of events that occured are:  %d\n",t_ev);
	fprintf(fp,"Total number of event based messages sent to base-station:  %d\n",t_b);
	fprintf(fp,"Total number of exit based messages sent to base-station:  %d\n",t_e);
	fprintf(fp,"Total number of messages exchanged between nodes are:  %d\n",t_n);
	fprintf(fp,"\n");
	fprintf(fp,"-----------------------------------------------------------------------------\n");



}

//write event info to log file
void baseStation_write_event_info(struct node base_recv,FILE * fp){
	
	fprintf(fp,"EVENT-INFO #### iteration - %d ,tStamp = %d ,ref_node = %d ,event_nodes = ", 																			base_recv.iteration_number + 1,base_recv.timeStamp,
											base_recv.my_rank);
	int i;
	for(i =0; i < 4; i++){
		if(base_recv.event_nodes[i] != -1){
			fprintf(fp,"%d,",base_recv.event_nodes[i]);
		}
	}
	fprintf(fp,"\n");

}

//send exit message to base station
void send_exit_to_base(struct node *n,MPI_Datatype mystruct){
	MPI_Send(n,1,mystruct,BASE_STATION_RANK,EXIT_TAG,MPI_COMM_WORLD);
}

//send event message to base station
void sendToBase_station(struct node *n,MPI_Datatype mystruct){
	MPI_Send(n,1,mystruct,BASE_STATION_RANK,EVENT_TAG,MPI_COMM_WORLD);
}

//create custom data type
void createCustom_data_type(MPI_Datatype *mystruct){

	const int nitems=5;
	int blocklengths[5] = {1,1,1,4,1};
	MPI_Datatype types[5] = {MPI_INT,MPI_INT,MPI_INT,MPI_INT,MPI_INT};
	MPI_Aint offsets[5];

	offsets[0] = offsetof(struct node,iteration_number);
	offsets[1] = offsetof(struct node,my_rank);
	offsets[2] = offsetof(struct node,messages_sent);
	offsets[3] = offsetof(struct node,event_nodes);
	offsets[4] = offsetof(struct node,timeStamp);
	
	MPI_Type_create_struct(nitems,blocklengths,offsets,types,mystruct);
	MPI_Type_commit(mystruct);

}


//create struct type
void prepare_Struct(int j,int ref_rank,int msg_sent,int nodes_inv[4],struct node *n,int nebo[4],int ts){

	int i;
	for (i = 0; i<4; i++){
		if(nodes_inv[i] == 0 && nebo[i] != -1){
			n->event_nodes[i] = nebo[i];
		}
		else{
			n->event_nodes[i] = -1;
		} 
	}
	n->iteration_number = j;
	n->my_rank = ref_rank;
	n->messages_sent = msg_sent;
	n->timeStamp = ts;
	
}


//check if event has occured
int checkIfEvent(int r_nums[4],int recv_tot,int n_i[4]){
	int i;
	int mismatch_allow = 1;
	if (recv_tot == 3 || recv_tot == 4){
		
		if((r_nums[0] == r_nums[1]) &&
			 (r_nums[1] == r_nums[2]) &&
			 (r_nums[2] == r_nums[3])){
				
				//printf(" YES EVENT\n");
				n_i[0] = 0;
				n_i[1] = 0;
				n_i[2] = 0;
				n_i[3] = 0;
				return 1;
		}
		
		else if((r_nums[0] == r_nums[1]) &&
						(r_nums[1] == r_nums[2])) {
						//printf(" YES EVENT\n");
						n_i[0] = 0;
						n_i[1] = 0;
						n_i[2] = 0;
						return 1;
		}
		
		else if((r_nums[0] == r_nums[1]) &&
						(r_nums[1] == r_nums[3])) {
						//printf(" YES EVENT\n");
						n_i[0] = 0;
						n_i[1] = 0;
						n_i[3] = 0;
						return 1;		
		}

		else if((r_nums[0] == r_nums[2]) &&
						(r_nums[2] == r_nums[3])) {
						//printf(" YES EVENT\n");
						n_i[0] = 0;
						n_i[2] = 0;
						n_i[3] = 0;
						return 1;
		}

		else if((r_nums[1] == r_nums[2]) &&
						(r_nums[2] == r_nums[3])) {
						//printf(" YES EVENT\n");
						n_i[1] = 0;
						n_i[2] = 0;
						n_i[3] = 0;
						return 1;		
		}

		else{
			//printf(" NO EVENT\n");
			return 0;	
		}

	}
	else{
		//printf(" NO EVENT\n");
		return 0;	
	}

}



//reset arrays
void reset(int r_nums[4]){
	int i;
	for(i = 0; i < 4; i++){
			r_nums[i] = -1;
	}
}
//send random numbers to neighbours
void send_r_num_to_neighbours(int myneighbours[4],int NNcount[4],int *r_n,MPI_Request *r){
	
	int i;
	int neighbour;

	int x = 0;
	for(i = 0; i < 4; i++){
		if(myneighbours[i] != -1 && NNcount[i] >= 3){
				neighbour = myneighbours[i];
				MPI_Isend(r_n,							//data - random number
								 1,								//number of elements
								 MPI_INT,					//data type
								 neighbour,				//destination rank
								 0,								//tag
								 MPI_COMM_WORLD,		//comm_world
								 &r[x]
								);
				x = x + 1;		
		}
	}
}

//receive random numbers from neighbours
void receive_r_num_from_neighbours(int total_n,int r_nums[4],int myneighbours[4],MPI_Request *r,int snd){

	if(total_n < 3) return;

	int i;
	int neighbour;
	int x = snd;
	for(i = 0; i < 4; i++){
		if(myneighbours[i] != -1){
				neighbour = myneighbours[i];
				MPI_Irecv(&r_nums[i],					//receive buffer
								 1,										//number of receive elements
								 MPI_INT,							//data type
								 neighbour,						//source rank
								 0,									  //tag
								 MPI_COMM_WORLD,			//comm_world
								 &r[x]							//status
								);
				 x = x + 1;	
		}
	}
	

}

void printStruct(struct node n){
	int i;
	printf(" event_nodes = ");
	for(i = 0; i< 4; i++){
		printf(" %d ",n.event_nodes[i]);
	}
	printf("\n");

}

void printRandArr(int r_nums[4]){
	int i;
	printf("neighbours sent = ");
	for(i = 0; i < 4; i++){
			printf("%d ", r_nums[i]);	
	}
	printf("\n");
}

//check if the program is needed to sleep
void checkSleep(double start, double end){
	
	int milisecond_time = (end-start)*1000;
	int diff_mili = iteration_length - milisecond_time;
	if(diff_mili > 0){
		int diff_sec = diff_mili / 1000;
		//printf(" hello - %d ",diff_sec);
		sleep(diff_sec);
	}
	//printf("elapsed time = %d\n",milisecond_time);
	
}


//fill 2-d grid with ranks of nodes
void fill_grid(int node_grid[n_rows][n_cols]){
	
	int row_cnt;
	int col_cnt;
	int rank = 1;
	for(row_cnt = 0; row_cnt < n_rows; row_cnt++){
		for(col_cnt = 0; col_cnt < n_cols; col_cnt++){
			node_grid[row_cnt][col_cnt] = rank;
			rank = rank + 1;
		}
	}

}

//find which row in 2-d grid the rank sits in
int find_row(int node_grid[n_rows][n_cols],int rank){
	
	int row_cnt;
	int col_cnt;
	int my_row;
	for(row_cnt = 0; row_cnt < n_rows; row_cnt++){
		for(col_cnt = 0; col_cnt < n_cols; col_cnt++){
			if(node_grid[row_cnt][col_cnt] == rank){
				my_row = row_cnt;
				return my_row;			
			}
		}
	}

}

//find neighbors of a particular node
int find_my_N(int rank,int neighbours[4],int node_grid[n_rows][n_cols]){

	int up,down,left,right,total_N;
	up = rank - 5;
	down = rank + 5;
	left = rank - 1;
	right = rank + 1;
	total_N = 0;

	int my_row = find_row(node_grid,rank);
	
	if(up >= 1 && up <= total_nodes){
		neighbours[0] = up;
		total_N = total_N + 1;
	}

	if(down >= 1 && down <= total_nodes){
		neighbours[1] = down;
		total_N = total_N + 1;
	}

	if(left >= 1 && left <= total_nodes){
		int left_row = find_row(node_grid,left);
		if(left_row == my_row){
			neighbours[2] = left;
			total_N = total_N + 1;
		}
	}

	if(right >= 1 && right <= total_nodes){
		int right_row = find_row(node_grid,right);
		if(right_row == my_row){
			neighbours[3] = right;
			total_N = total_N + 1;
		}
	}
	
	return total_N;	

}




int find_N_N(int neighbour_rank,int node_grid[n_rows][n_cols]){

	int up,down,left,right,total_N;
	up = neighbour_rank - 5;
	down = neighbour_rank + 5;
	left = neighbour_rank - 1;
	right = neighbour_rank + 1;
	total_N = 0;

	int my_row = find_row(node_grid,neighbour_rank);
	
	if(up >= 1 && up <= 20) total_N = total_N + 1;
	if(down >= 1 && down <= 20)total_N = total_N + 1;
	if(left >= 1 && left <= total_nodes){
		int left_row = find_row(node_grid,left);
		if(left_row == my_row){
			total_N = total_N + 1;
		}
	}

	if(right >= 1 && right <= total_nodes){
		int right_row = find_row(node_grid,right);
		if(right_row == my_row){
			total_N = total_N + 1;
		}
	}
	return total_N;	
}


void fill_N_N_count(int myneighbours[4],int NNcount[4],int node_grid[n_rows][n_cols]){
	int i;
	for(i = 0; i < 4; i++){
		if(myneighbours[i] != -1) NNcount[i] = find_N_N(myneighbours[i],node_grid);	
	}
}


int find_send_valid_N(int myneighbours[4],int NNcount[4]){
	int i;
	int count = 0;
	for(i = 0; i < 4; i++){
		if(myneighbours[i] != -1 && NNcount[i] >=3) count = count + 1;
	}
	return count;
}

int find_recv_valid_N(int total_neighbours){
	if(total_neighbours >= 3) return total_neighbours;
	else return 0;
}

void print_N(int neighbours[4],int rank){
	printf("\n");
	printf("rank = %d, neighbours = ",rank);
	int i;
	for(i = 0; i < 4; i++){
		if(neighbours[i] != -1){
			if(i == 0) printf(" up = %d ",neighbours[i]);
			if(i == 1) printf(" down = %d ",neighbours[i]);
			if(i == 2) printf(" left = %d ",neighbours[i]);
			if(i == 3) printf(" right = %d ",neighbours[i]);
		}
	}
	printf("\n");
}

