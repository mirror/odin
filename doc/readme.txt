I am writing directly to a disk (opend with eg. "physicaldrive1") and
want to reject all other accesses to that particular disk and to all
logical volumes possibly residing on it.

A FSCTL_LOCK_VOLUME issued on the disk handle returns immediately with
success, while files on a logical drive on that disk are still in use.

So Ok, what I need is a LOCK_ENTIRE:DISK or so ..., LOCK_VOLUME is the
wrong call :) - but I did not find any other ..

Anyone an idea how to do that?


tia
w.b. 

Valeriy
Tue Apr 04 05:45:01 CDT 2006

Hi,

Simple sending FSCTL_LOCK_VOLUME to the child volumes is not enough to get
control over a physical drive.

The process should be as following:
0) Open the physical drive exclusively for R/W
1) Find all the volumes related to the hard drive taking care of software
RAID partitions etc.
2) Open each child volume in read/write shared mode and try to lock it with
FSCTL_LOCK_VOLUME.
3) If all the child volumes have been locked, dismount them one by one with
FSCTL_DISMOUNT_VOLUME.
4) Don't close the volumes' handles!
5) Use the whole hard drive exclusively as you like.
6) Close all the volume handles
7) Refresh the physical drive's layout if needed
8) Close the main handle to the drive
9) The child volumes will be remounted by the system once they are accessed
again.

I hope this helps.

-- 
Best regards,
Valeriy Glushkov