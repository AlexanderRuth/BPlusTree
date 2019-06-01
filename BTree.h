/*
*	BPlusTree.h
*
*	By: Alex Ruth
*	Github: AlexanderRuth
*
*	Templated C++ implementation of a B+ Tree
*	View documentation at https://github.com/AlexanderRuth/BPlusTree
*/

#ifndef BTREE_H
#define BTREE_H

#include <vector>
#include <tuple>
#include <iostream>

#define BLOCK_SIZE 4096 //Block size in bytes

using namespace std;

template <class K, class T>
class BTree {

	public:
		
		struct DataNode_t; //Forward declare

		//This is the meat of each node. Each node is a vector of
		//data_entry's.
		//
		// 0 = Pointer to left node
		// 1 = Key for the given entry
		// 2 = Pointer to actual data (only for leaf nodes)
		// 3 = Pointer to right node
		typedef tuple<struct DataNode_t*, K, T*, struct DataNode_t*> data_entry;
		
		//A struct that holds a vector of data entries
		//Local to the given templated version of the BTree

		struct DataNode_t{
			vector< data_entry > m_entries; //List of data_entry's
			int m_num_entries;		//Number of valid entries
			struct DataNode_t* m_right;	//Pointer to its right neighbor
			int m_max_entries;		//Max number of possible entries
			bool m_is_leaf;			//Is the node a leaf?

			//Constructor
			//
			//Takes max number of entries, and sets values to default
			DataNode_t(int max_entries, bool is_leaf) 
				: m_right(NULL), m_num_entries(0), m_is_leaf(is_leaf)
			{
				m_entries.resize(max_entries);
				m_max_entries = max_entries;
			}

			//next()
			//
			//Get a pointer to the next level of nodes based on the key 
			//NOTE: Returns NULL if at a leaf
			struct DataNode_t* next(K key)
			{
				int i;		//Loop counter
				K cur_key;	//Current key being checked
			
				//If there are no entries, then the tree is empty
				//and this is the root
				if(m_num_entries == 0)
					return NULL;

				//Find the pointer in the middle of two keys
				for(i = 0; i < m_num_entries; i++){
					cur_key = get<1>(m_entries.at(i));
					
					if(cur_key > key)
						return get<0>(m_entries.at(i));
					if(cur_key == key)
						return get<3>(m_entries.at(i));
				}

				//If greater than every key, return rightmost pointer
				i--;
				return get<3>(m_entries.at(i));			
			}

			//retrieve()
			//
			//Retrieve a T pointer if it exists in the given node
			//NOTE: Should only be used on leaf nodes
			T* retrieve(K key)
			{
				int i;
				K cur_key;
				
				if(m_num_entries == 0)
					return NULL;
					
				for(i = 0; i < m_num_entries; i++)
					if(key == get<1>(m_entries.at(i)))
						return get<2>(m_entries.at(i));
				
				return NULL;
			}

			//insert()
			//
			//Insert an entry, returns true if succesful,
			//false otherwise
			bool insert(data_entry entry)
			{
				int i;		//Loop counter
				K cur_key;	//Current node key
				int index;	//Found index

				//If there are no entries, insert at index 0
				if(!m_num_entries){
					index = 0;
				}

				//Otherwise, find correct location
				else{
			
					//Search for index
					for(i = 0; i < m_num_entries; i++){
						cur_key = get<1>(m_entries.at(i));
					
						if(get<1>(entry) <= cur_key)
							break;
					}

					index = i;
				}

				//Insert the entry at the given index
				m_num_entries++;
				m_entries.insert(m_entries.begin()+index, entry);

				//Update the pointers to the left and right entries,
				//if they exist
				if(index != 0)
					get<3>(m_entries.at(index-1)) = get<0>(entry);
				if(index != m_num_entries-1)
					get<0>(m_entries.at(index+1)) = get<3>(entry);

				//Insert successful!
				return true;
			}

			//mid_entry()
			//
			//Returns he middle entry of the data node
			data_entry mid_entry(){ return m_entries.at(m_num_entries / 2 + 1); }

			//is_full()
			//
			//Is the data node full?
			bool is_full() {return m_num_entries >= m_max_entries;}

			//inorder()
			//
			//Print the entries in the node in sorted order
			void inorder()
			{
				for(int i = 0; i < m_num_entries; i++)
					cout << "[i=" << i << " k=" << get<1>(m_entries.at(i)) << "]";
				cout << endl;
			}

		};
		typedef struct DataNode_t DataNode;

		//Public methods		

		BTree();					//Default Constructor
		BTree(int);					//Constructor with user defined block size
		bool insert(K, T*);				//Insert into the B+ Tree
		void inorder(int);				//Print the contents of the B+ Tree
		T* retrieve(K);					//Get a pointer to a given entry
		int depth();					//Return the depth of the tree
		~BTree();					//Deconstructor

	private:
		
		//Members

		DataNode *m_root;					//Root of the tree
		const int m_block_size;					//Block size for each node
		const int m_max_entries;				//Max entries for each node
		int m_depth;						//Current depth (starts at 0)

		//Super secret methods

		bool insert_recursive(DataNode*, K, T*, data_entry&);	//Recursive insert call
		T* retrieve_recursive(K, DataNode*);			//Recursive retrieve call
		void split(DataNode*, bool, data_entry&);		//Split a given node
		void delete_recursive(DataNode*);			//Recursive deconstruction
};

//Default constructor
//
//Use a default block size
template <class K, class T>
BTree<K,T>::BTree()
	: m_block_size(BLOCK_SIZE), 
	  m_max_entries(BLOCK_SIZE / sizeof(data_entry)),
	  m_depth(0)
{
	m_root = new DataNode(m_max_entries, true);
}

//Specific Constructor
//
//Use the specified block size
template <class K, class T>
BTree<K,T>::BTree(int block_size)
	: m_block_size(block_size),
	  m_max_entries(block_size / sizeof(data_entry)),
	  m_depth(0)
{
	m_root = new DataNode(m_max_entries, true);
}

//~BTree()
//
//Deconstructor
template <class K, class T>
BTree<K,T>::~BTree()
{
	if(!m_root->m_is_leaf)
		delete_recursive(get<0>(m_root->m_entries.at(0)));

	delete m_root;
}

//delete_recursive()
//
//Recursive deletion, used by deconstructor
template <class K, class T>
void BTree<K,T>::delete_recursive(DataNode* curr)
{
	DataNode* tmp;

	if(!curr->m_is_leaf){
		delete_recursive(get<0>(curr->m_entries.at(0)));
		
		while(curr->m_right){
			tmp = curr->m_right;
			curr->m_entries.clear();
			delete curr;
			curr=tmp;
		}
		delete curr;
	}
	else{
		while(curr->m_right){
			tmp = curr->m_right;
			curr->m_entries.clear();
			delete curr;
			curr=tmp;
		}

		curr->m_entries.clear();
		delete curr;
	}
}

//inorder()
//
//Print the contents of the tree
template <class K, class T>
void BTree<K,T>::inorder(int min)
{
	DataNode* curr;		//Current node
	DataNode* right;	//Node to the right
	int i = 0;		//Loop counter

	//Start a root
	curr = m_root;
	
	//Output depth along the way
	cout << "\nDEPTH=0" << endl << endl;
	curr->inorder();

	//While not at leaf
	while(curr->next(min)){

		//Output current level
		i++;
		curr = curr->next(0);
		cout << "\nDEPTH=" << i << endl << endl;
		curr->inorder();

		//Move along the right
		right = curr->m_right;
		while(right){
			right->inorder();
			right = right->m_right;
		}	
	}
			
	cout << endl;
}

//insert()
//
//Insert the given data based on the key
template <class K, class T>
bool BTree<K,T>::insert(K key, T *data)
{
	data_entry new_entry; //The entry to add
	
	//Set up the entry to add
	get<1>(new_entry)=key;
	get<2>(new_entry)=data;


	//If split propagates to the top of the tree,
	//make a new root
	if(insert_recursive(m_root, key, data, new_entry)){
		m_root = new DataNode(m_max_entries, false);
		m_root->insert(new_entry);

		m_depth++;

		return true;
	}

	return true;
}

//insert_recursive()
//
//Private. Recursive portion of the insert method
template <class K, class T>
bool BTree<K,T>::insert_recursive(DataNode *curr, K key, T *data, data_entry &entry)
{
	
	//If not at leaf, keep recursing
	if(curr->next(key)){
	
		//If split propagates up, insert entry and
		//split again as needed
		if(insert_recursive(curr->next(key), key, data, entry)){

			if(curr->is_full()){
				split(curr, false, entry);
				return true;
			}
			else{
				curr->insert(entry);
				return false;
			}
		}

		//Otherwise, did not split
		return false;
	}
	
	else{
		//At a leaf node, insert a new entry
		//If full, split, otherwise,
		//simply insert
		if(curr->is_full()){
			split(curr, true, entry);
			return true;
		}

		else{
			curr->insert(entry);
			return false;
		}
	}
}

//retrieve()
//
//Public wrapper around retrieve. Returns a pointer
//to the desired entry, or NULL if it does not exist
template <class K, class T>
T* BTree<K,T>::retrieve(K key)
{
	//Start at root
	return retrieve_recursive(key, m_root);	
}

//retrieve_recursive()
//
//Recursive retrieve function
template <class K, class T>
T* BTree<K,T>::retrieve_recursive(K key, DataNode* curr)
{
	//Keep going until at leaf, then retrieve
	//based on the given key
	if(!curr->next(key))
		return curr->retrieve(key);
	else
		return retrieve_recursive(key, curr->next(key));
}

//split()
//
//Split the passed DataNode into two, and set new_entry to the upwards
//propagated data entry
template <class K, class T>
void  BTree<K,T>::split(DataNode* to_split, bool is_leaf, data_entry &new_entry)
{
	int half_size = (m_max_entries) / 2;			//Half the size of DataNode
	K mid_key;						//The key in the middle
	DataNode* right_node = 
		new DataNode(m_max_entries, is_leaf);		//Allocate a new node

	
	//Get the middle key
	mid_key = get<1>(to_split->m_entries.at(half_size));

	//Make the new node the right of the two, and fill its entries
	//If this is not the leaf node, ditch the middle key
	right_node->m_entries = vector<data_entry>(
		to_split->m_entries.begin() + half_size + (is_leaf ? 0 : 1), 
		to_split->m_entries.begin() + m_max_entries
	);

	//Set proper member variables
	right_node->m_num_entries = right_node->m_entries.size();
	right_node->m_entries.resize(m_max_entries);
	right_node->m_right = to_split->m_right;

	//Make the original node the left of the two, and fill its entries
	to_split->m_entries = vector<data_entry>(
		to_split->m_entries.begin(),
		to_split->m_entries.begin() + half_size
	);

	//Set proper member variables
	to_split->m_num_entries = to_split->m_entries.size();
	to_split->m_entries.resize(m_max_entries);
	to_split->m_right = right_node;

	//Insert the new entry in the proper node of the two
	if(mid_key < get<1>(new_entry))
		right_node->insert(new_entry);	
	else
		to_split->insert(new_entry);

	//Set up the new entry to propagate upwards.
	//It points to the two split nodes, and its
	//key was the middle key of the original node
	get<0>(new_entry)=to_split;
	get<1>(new_entry)=mid_key;
	get<3>(new_entry)=right_node;
}

//depth()
//
//Return the depth of the tree
template <class K, class T>
int BTree<K,T>::depth() {return m_depth;}

#endif
