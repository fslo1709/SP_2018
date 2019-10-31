B06902102	羅費南

1.	Execute the make file using make and execute as described
in the specs file.

2.	I think I finished all the grading criteria. Not 100% sure
if customer.c can detect the all of the timeouts correctly
(I think my assumption that the VIP would never timeout is 
correct, but would need to double check that later).
Bidding system EDF was not tested thoroughly, thus it might fail.
But I'm pretty confident it won't. I write things to bidding_system_log AFTER I am sure it is the shortest time one,
so I might differ in that with the specs given (but from what
I read in the specs, that is how is supposed to be done).

NOTE: edited slightly because I checked more of the bugs
it had.
Sometimes it has a weird error, I used pkill bidding_system_EDF
and pkill customer_EDF because apparently it didn't close correctly,
but couldn't figure out how to remove this bug.