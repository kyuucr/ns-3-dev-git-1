The aim of this folder is to test multi-radio when there is changes in the topology
We have a 3x3 network:
7-8-9
4-5-6
1-2-3

The nodes 2-3,5-6,8-9 are a 3x2 mesh. 4 is connected to 5, and 1 can connect with 2 or 5, 7 can connect with 5 or 8.
Nodes 2 and 8 connects with the satellite. Changes in the topology:
Time a: node 1 connected to node 2, and 7 to 8.
Time b: node 1 connected to node 5, and 7 to 8. (one change)
Time c: node 1 connected to 2, and 7 to 5. (two changes)
