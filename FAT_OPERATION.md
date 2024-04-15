# Overview of FAT file system update operations

## Basic Operation

### Create file

1. Get a cluster that is not assigned to the FAT table
2. Write file block to cluster
3. Update the consumed cluster in the FAT table
4. Add a directory entry for the file (with new cluster id)

### Update file

1. Get a cluster that is not assigned to the FAT table
2. Write a file block to the cluster
3. Release assigned cluster in the FAT table, and update the consumed cluster
4. Update the directory entry for the file (with cluster id)

### delete file

1. `DIR_Name[0] = 0xE5` of the directory entry (no deletion)
2. Release the assigned cluster from the FAT table


## Sequence for macOS Connection

The file ".fseventsd/fseventsd-uuid" is created.

1. A buffer cleared to zero is written into the first cluster (cluster=2). The FAT table is updated to include the consumption of cluster=2.
2. Directory entry data within ".fseventsd" is written into cluster=2.
3. An entry for ".fseventsd" (cluster=2) is added to the root directory entry.
4. Data for ".fseventd/fseventsd-uuid" is written into cluster=3 (name not yet assigned). The FAT table is updated to include the consumption of cluster=3.
5. The directory entry in cluster=2 is updated to include "fseventsd-uuid" at cluster=3.

Reflect the change in the root directory entry in step 3 on the local filesystem (in step 3, execute `mkdir .fseventsd`).
Reflect the change in the subdirectory entry in step 5 on the local filesystem (in step 5, create the "fseventsd-uuid" file and write data to cluster 3).

New files and directories are created using free clusters in the FAT table. This enables the transmitted anonymous buffer to be identified by its block ID (block ID -1 == cluster id), determining whether it updates an existing resource or creates a new one. Updates to existing files are performed by registering the data in a new cluster and switching the directory entry's cluster id.
