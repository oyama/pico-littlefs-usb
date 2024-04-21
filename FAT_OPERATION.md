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

or, In Windows 11 do the following

1. Get a cluster that is not assigned to the FAT table
2. Release assigned cluster in the FAT table, and update the consumed cluster
3. Update the directory entry for the file (with cluster id)
4. Write a file block to the cluster

### delete file

1. `DIR_Name[0] = 0xE5` of the directory entry (no deletion)
2. Release the assigned cluster from the FAT table

### Rename

Name change in the same directory. clusters do not change.

1. Add a new file entry to the old file entry and set the delete flag (0xE5) to the old file entry.

or, In Windows 11 do the following

1. Update new name for the target directory entry

### Move

Moving to a different directory. clusters do not change.

1. set delete flag (0xE5) to the old file entry
2. add new file to directory entry.
