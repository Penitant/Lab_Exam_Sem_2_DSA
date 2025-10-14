import sys
import json
import os
import re


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

    def to_list(self):
        out = []
        cur = self.h
        while cur:
            out.append(cur.v)
            cur = cur.n
        return out

    @staticmethod
    def from_list(items):
        q = Queue()
        for x in items:
            q.enqueue(x)
        return q


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
        stack = []
        cur = self.root
        while stack or cur:
            while cur:
                stack.append(cur)
                cur = cur.l
            cur = stack.pop()
            yield cur.w, cur.q.to_list()
            cur = cur.r


def save_bst_json(root, path):
    ids = {}
    nodes = []

    def visit(n):
        if n is None:
            return None
        if n in ids:
            return ids[n]
        nid = len(ids)
        ids[n] = nid
        left_id = visit(n.l)
        right_id = visit(n.r)
        nodes.append({
            "id": nid,
            "w": n.w,
            "lines": n.q.to_list(),
            "l": left_id,
            "r": right_id,
        })
        return nid

    root_id = visit(root) if root else None
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"root": root_id, "nodes": nodes}, f, ensure_ascii=False, indent=2, sort_keys=False)


def load_bst_json(path):
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    if not data or data.get("root") is None:
        return None
    by_id = {}
    for rec in data["nodes"]:
        n = Node(rec["w"])
        n.q = Queue.from_list(rec.get("lines", []))
        by_id[rec["id"]] = n
    for rec in data["nodes"]:
        n = by_id[rec["id"]]
        n.l = by_id.get(rec.get("l"))
        n.r = by_id.get(rec.get("r"))
    return by_id[data["root"]]


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


def main(argv):
    if len(argv) < 2 or len(argv) > 4:
        sys.stderr.write(
            "Usage: python BSTGenerator.py <input.txt> <output_index.txt> [bst.json] [output_bst.json]\n"
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
        root = load_bst_json(input_bst)
        bst.root = root

    build_index(bst, input_txt)
    write_index(bst, output_idx)

    if output_bst:
        save_bst_json(bst.root, output_bst)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

