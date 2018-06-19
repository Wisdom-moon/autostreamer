Functions that need to implement.

Done:
  Scalar variables that is assigned value at the begin of kernel, we have no
    need to transfer its value. 
  Non-int type variable can not direct convert to uint64, otherwise its value change.
  The hstream init code should be placed at the beginning of function. But the memory bufffer must be created after its host mem declared. And hstream finish code should be placed at the end of function.
  Compare expressions' value. We can assignment each variable 2(0, 1 may not suitable for add/mul operator), and compute its value, then compare to find which is larger.

Need to do:
  Recognize array's size at its define "TYPE Array[xx][xx]";
  Analysis array's indexes, to find the overlap.
  Array that is assigned value at the begin of kernel, no need to transfer h2d. The problem is we need to make sure all element used later is assigned value in kernel!
  Analysis where the array is used after kernel, can push the transferment from dev to host later.
  Two or more level index, can not infer the size of array.
  Optimization for redundant memory transfer.
  Optimization for memory access.
  When evaluate index, it is calculated with a statement, it is hard to known whether wo should replace the vars in that statement with its evaluated value. The situation is too complex!
  Eliminate redundant parentheses.
  Can not know which variable is bigger in different indices.
