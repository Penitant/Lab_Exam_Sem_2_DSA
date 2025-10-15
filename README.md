# Index Generator – Lab Examination 2

Utility to index words in a text file and output them in lexicographic order with the list of line numbers where each word appears. The project has two implementations:

- Python: Binary Search Tree (BST) with on-disk persistence (store/reload the tree)
- C: Robin Hood hashing (no persistence required for the hash table)

Both implementations scan the input file, tokenize words using ASCII order and common separators, and produce a word index file.

## Assignment summary (Chanakya University)

You will implement an index generator that:

- Reads a text file, extracts words (ASCII-ordered comparison), and outputs all distinct words in lexicographic order with the list of line numbers where each word occurs.
- Uses a BST as the primary data structure in Python. Each BST node stores the word and a queue (linked list) of line numbers. Repeated words are allowed; the queue must handle duplicates correctly (no fixed-size arrays).
- Supports persistence for the BST: you should be able to store the BST (including line lists) to a file, later reload it, and continue index generation on the same tree.
- Also implements index generation using Robin Hood hashing in C with a suitable hash function, and compares performance (time) with the BST version. The hash table itself need not be persisted to disk, but the resulting index should be output to a file.
- CLI arguments: input text file name, output index file name, an optional partially constructed BST file to load, and an optional output BST file name to save the resulting tree.

Deliverables: design explanation (this README), code, and test cases. During examination, you may be asked to modify your own code to add features.

## Python implementation (BST with persistence)

Location: `IndexGen_Py/BSTGenerator.py`

Key points:

- Data structures:
	- `BST` with `Node` containing: `w` (word), `l` (left), `r` (right), and `q` (a linked-list queue of line numbers).
	- The queue is a simple singly linked list that appends new line numbers and coalesces immediate duplicates on the same line.
- Tokenization: words are split on spaces, tabs, newlines, and the punctuation characters `.` `,` `:` `;` (ASCII separators). Comparison is by ASCII order.
- Index output: in-order traversal of the BST writes one line per word, followed by the (deduplicated-per-line) list of line numbers.
- Persistence: the BST can be saved to and loaded from JSON.

### JSON format for the BST (how the tree is stored)

BSTs are serialized as an adjacency-like structure that preserves the exact tree shape, not as a flat list. The schema is:

```
{
	"root": <root_node_id | null>,
	"nodes": [
		{
			"id": <number>,       // unique integer id for this node
			"w": <string>,        // the word (key)
			"lines": <number[]>,  // queue of line numbers, in insertion order (no immediate duplicates)
			"l": <id | null>,     // left child node id
			"r": <id | null>      // right child node id
		},
		...
	]
}
```

- `root` gives the id of the root node (or `null` for an empty tree).
- Each node references its left/right children by id, so when loading we reconstruct the pointer structure and recover the same BST.
- The `nodes` array order corresponds to a traversal during serialization; it is not guaranteed to be sorted by id or by key.

In short: the JSON stores one root id plus a list of node records with `l`/`r` references — it is the BST structure, not a “flat root+all words” dump.

### CLI

Run from the repository root (examples):

```
python3 IndexGen_Py/BSTGenerator.py <input.txt> <output_index.txt> [existing_bst.json] [output_bst.json]
```

- If only two arguments are provided, the program builds a new BST, writes the index, and does not save the BST.
- With three arguments, if the third path exists it is treated as an input BST to load and extend; otherwise it is used as the output BST save path.
- With four arguments, it loads from the third JSON and saves to the fourth JSON.

Example:

- Build fresh and save tree: `python3 IndexGen_Py/BSTGenerator.py "Text Files/compiler-design-essay.txt" compiler_index.txt compiler_bst.json`
- Continue from saved tree and write a new index: `python3 IndexGen_Py/BSTGenerator.py "Text Files/literature_essay.txt" literature_index.txt compiler_bst.json combined_bst.json`

## C implementation (Robin Hood hashing)

Location: `IndexGen_C/index_generator.c`

Key points:

- Uses Robin Hood hashing with probe distance and linear probing.
- Each hash table entry stores a word and a linked list of line numbers (sorted and deduplicated).
- Tokenization uses the same separator set as the Python version.
- At the end, entries are collected and sorted lexicographically to generate the index file.
- The program prints a simple timing for performance comparison.

### CLI

Compile with a standard C compiler, then run:

```
gcc -O2 -o index_c IndexGen_C/index_generator.c
./index_c "Text Files/compiler-design-essay.txt" compiler_index_hash.txt
```

For a quick performance comparison, index the same file with both implementations and compare elapsed time (the C version prints timing; you can time the Python run using your shell if desired).

## Word separators and order

- Separators: one or more of space, tab, newline, full stop (.), comma (,), colon (:), or semicolon (;).
- Ordering: ASCII lexicographic order. Non-ASCII punctuation (e.g., an em-dash) will be treated as part of a token by the current tokenizer.

## Repository layout

- `IndexGen_Py/` – Python BST implementation and sample JSONs
- `IndexGen_C/` – C Robin Hood hashing implementation
- `Text Files/` – Sample input essays

## Notes on text files formatting

The sample essays in `Text Files/` are already structured as readable paragraphs with blank lines between them. No content changes were made. If you prefer different wrapping (e.g., ~80–100 columns) or stricter ASCII-only punctuation, we can reflow the text on request.

## Testing ideas

- Happy path: run both implementations on the provided essays and verify that the word lists and line numbers are sensible.
- Edge cases: empty file, file with only separators, repeated words on the same line (should not duplicate the line number in Python’s queue), and very long words.
- Persistence: in Python, save a BST after partially indexing one file, then load it and continue with a second file; confirm the combined index and JSON load/save cycle are consistent.
