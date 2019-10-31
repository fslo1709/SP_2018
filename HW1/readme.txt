B06902102


Description: The following program is a code to manage simple requests
done by clients to a server code that can manage a very simple bidding system.

The operations supported by this code are: buy, sell and price

These operations were explained as part of the assignment, so they will not 
be explained in here.

Code description:

Inside the while loop, which manages the server, it begins by using the select
operation to determine if there is a new stream of data incoming to the server
If the data is new, it assigns the connection using accept and adds this file 
descriptor to LARGEST, which is the fd_set that manages all file descriptors 
available. This is done in a if condition to check if the server is the one
that is changing data. If not, it means one of the clients is the one sending data,
so it loops through all of the possible clients to see if any of those is the one 
sending the data. If it is, it proceeds to do the necessary work with the data that
just arrived. 

1. The program compiles.
2. By reading the buf input, it searches for that item in item_list
and returns the appropiate value
3. By reading the buf input, it changes the file according to the
direction given by the user.
4. Using select, it is able to detect input changes and read the desired element 
for each client accordingly.
5. Using select, it is able to manage different lines of input so it can detect which element each distinct user
wants to modify. It uses wait_for_read to distinguish which type of data it wants to receive from the client.
6. By using an array of integers (basically flags), it can keep track if any data is being read by a client per each item
in the bidding system, and such it can block any client from reading data from the same piece of the file (this is the
solution that worked for me, since fcntl only worked for me with different servers)
7. File advisory lock protects the file from having the same data being modified by two different clients on the same or different servers
This is achieved using fcntl() to lock and unlock the file after usage.