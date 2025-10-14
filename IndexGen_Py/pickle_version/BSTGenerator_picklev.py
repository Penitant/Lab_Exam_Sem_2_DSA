import sys
import os
import re
import pickle


class QNode:
	def __init__(self, v):
		self.v = v
		self.n = None


class Queue:
	def __init__(self):
		self.h = None
		self.t = None

	def enqueue(self, v):
		if self.t and self.t.v == v:
			return
		qn = QNode(v)
		if not self.h:
			self.h = self.t = qn
		else:
			self.t.n = qn
			self.t = qn


class Node:
	def __init__(self, w):
		self.w = w
		self.l = None
		self.r = None
		self.q = Queue()


class BST:
	def __init__(self):
		self.root = None

	def insert(self, w, line):
		def ins(n, w, line):
			if not n:
				n = Node(w)
				n.q.enqueue(line)
				return n
			if w < n.w:
				n.l = ins(n.l, w, line)
			elif w > n.w:
				n.r = ins(n.r, w, line)
			else:
				n.q.enqueue(line)
			return n

		self.root = ins(self.root, w, line)

	def inorder(self):
		s = []
		c = self.root
		while s or c:
			while c:
				s.append(c)
				c = c.l
			c = s.pop()
			yield c.w, list_iter_queue(c.q)
			c = c.r


def list_iter_queue(q):
	out = []
	cur = q.h
	while cur:
		out.append(cur.v)
		cur = cur.n
	return out


def tokenize_line(s):
	parts = re.split(r"[ \t\n\.,:;]+", s)
	return [p.lower() for p in parts if p]


def build_index(bst, input_path):
	with open(input_path, "r", encoding="utf-8", errors="ignore") as f:
		for i, line in enumerate(f, start=1):
			for w in tokenize_line(line):
				bst.insert(w, i)


def write_index(bst, out_path):
	with open(out_path, "w", encoding="utf-8") as f:
		for w, lines in bst.inorder():
			f.write(f"{w}: {' '.join(str(x) for x in lines)}\n")


def save_bst_pickle(root, path):
	with open(path, "wb") as f:
		pickle.dump(root, f, protocol=pickle.HIGHEST_PROTOCOL)


def load_bst_pickle(path):
	with open(path, "rb") as f:
		return pickle.load(f)


def main(argv):
	if len(argv) < 2 or len(argv) > 4:
		sys.stderr.write(
			"Usage: python BSTGenerator_picklev.py <input.txt> <output_index.txt> [bst.pkl] [output_bst.pkl]\n"
		)
		return 1
	input_txt = argv[0]
	output_idx = argv[1]
	input_bst = None
	output_bst = None
	if len(argv) == 3:
		third = argv[2]
		if os.path.exists(third):
			input_bst = third
		else:
			output_bst = third
	elif len(argv) == 4:
		input_bst = argv[2]
		output_bst = argv[3]

	bst = BST()
	if input_bst and os.path.exists(input_bst):
		bst.root = load_bst_pickle(input_bst)

	build_index(bst, input_txt)
	write_index(bst, output_idx)

	if output_bst:
		save_bst_pickle(bst.root, output_bst)
	return 0


if __name__ == "__main__":
	raise SystemExit(main(sys.argv[1:]))
