Functions that need to implement.

1. Recognize array's size at its define "TYPE Array[xx][xx]";
2. Scalar variables that is assigned value at the begin of kernel, we have no
   need to transfer its value.  Done!
3. Analysis array's indexes, to find the overlap.
4. Non-int type variable can not direct convert to uint64, otherwise its value change.
