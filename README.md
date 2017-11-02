## Association Rule Mining (ARM) for (pseudo)recommender system!
This repository contains some common algorithms for Association Rule Mining.
Currently Apriori algorithm is implemeneted.


###Apriori
The code for **Apriori** algorithm is implemented in C++.
This impelmentation uses **Trie**(prefix tree) for speed-up!
The original Apriori paper can be found at http://www.vldb.org/conf/1994/P487.PDF

#### Usage
The transaction file needs to be with this format:
	1. One transaction per a line
	2. The items in each transaction should be **sorted**
