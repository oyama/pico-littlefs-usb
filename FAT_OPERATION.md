# Overview of FAT file system update operations

## Create file

1. Get a cluster that is not assigned to the FAT table
2. Write file block to cluster
3. Update the consumed cluster in the FAT table
4. Add a directory entry for the file (with new cluster id)

## Update file

1. Get a cluster that is not assigned to the FAT table
2. Write a file block to the cluster
3. Release assigned cluster in the FAT table, and update the consumed cluster
4. Update the directory entry for the file (with cluster id)

## delete file

1. `DIR_Name[0] = 0xE5` of the directory entry (no deletion)
2. Release the assigned cluster from the FAT table