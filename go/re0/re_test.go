package re0

import (
	"testing"
)

func TestReToNFA(t *testing.T) {
	nfa := REtoNFA("a*bc")
	nfa_alloc_id(nfa.Start, 1)
	nfa.End.accept = true
	dfa := nfa_to_dfa(nfa)
	t.Log("state", "a", "b", "c", "accept")
	for i, ln := range dfa.tran {
		t.Log(i, "   ", ln['a'], ln['b'], ln['c'], dfa.accept[i])
	}
	t.Log(dfa.match("abc"))
}

func TestMultiRE(t *testing.T) {
	nfa1 := REtoNFA("a*bc")
	nfa2 := REtoNFA("a*cb")
	nfa := &NFA{Start: &NFAState{}, End: &NFAState{}}
	nfa.Start.AddE(nfa1.Start)
	nfa1.End.AddE(nfa.End)
	nfa.Start.AddE(nfa2.Start)
	nfa2.End.AddE(nfa.End)
	nfa_alloc_id(nfa.Start, 1)
	nfa.End.accept = true
	dfa := nfa_to_dfa(nfa)
	t.Log("state", "a", "b", "c", "accept")
	for i, ln := range dfa.tran {
		t.Log(i, "   ", ln['a'], ln['b'], ln['c'], dfa.accept[i])
	}
	t.Log(dfa.match("abc"))
	t.Log(dfa.match("acb"))
}

func TestMatch(t *testing.T) {
	t.Log(Match("a*bc", "abc"))
}
