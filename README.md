# Value Invariant Property
The repo ensures the value invariant properties via Information hiding

## Disclaimer: this is not hyperspace
This repo is not the same as [HyperSpace][1].  
Specifically, this repo made the following changes.  

- This repo only implements Code Pointer Seperation (CPS), which allows sophistitcated attacks for unchecked pointers.
- To protect runtime defense metadata, this prototype uses information hiding instead of Intel MPK.  
   The reason is that my machine does not support Intel MPK.
- [HyperSpace][1] leverages kernel changes to map memory at a fixed location for efficient lookup. However, this work simply change the compiler tool. Therefore, 
we uses two level-table lookup.
- No heap VIP implementations. (TODO)   


## references
[HyperSpace](https://dl.acm.org/doi/pdf/10.1145/3460120.3485376)

[1]: <https://dl.acm.org/doi/pdf/10.1145/3460120.3485376> "HyperSpace"
