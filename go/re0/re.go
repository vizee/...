package re0

func REtoNFA(exp string) *NFA {
	priority := map[byte]int{
		'(': 1,
		'|': 2,
		'&': 3,
	}
	var (
		opstack  []byte
		nfastack []*NFA
	)
	apply_op := func(op byte) {
		switch op {
		case '*':
			nfastack[len(nfastack)-1] = nfa_star(nfastack[len(nfastack)-1])
		case '&':
			nfastack[len(nfastack)-2] = nfa_cat(nfastack[len(nfastack)-2], nfastack[len(nfastack)-1])
			nfastack = nfastack[:len(nfastack)-1]
		case '|':
			nfastack[len(nfastack)-2] = nfa_or(nfastack[len(nfastack)-2], nfastack[len(nfastack)-1])
			nfastack = nfastack[:len(nfastack)-1]
		}
	}
	add_op := func(op byte) {
		switch op {
		case '*':
			apply_op('*')
		case ')':
			for len(opstack) > 0 {
				top := opstack[len(opstack)-1]
				opstack = opstack[:len(opstack)-1]
				if top == '(' {
					break
				}
				apply_op(top)
			}
		case '&', '|':
			if len(opstack) > 0 {
				prio := priority[op]
				for len(opstack) > 0 {
					top := opstack[len(opstack)-1]
					if priority[top] < prio {
						break
					}
					opstack = opstack[:len(opstack)-1]
					apply_op(op)
				}
			}
			opstack = append(opstack, op)
		case '(':
			opstack = append(opstack, op)
		}
	}
	cat := false
	escape := false
	for i := 0; i < len(exp); i++ {
		c := exp[i]
		if escape {
			switch c {
			case 'n':
				c = '\n'
			case 't':
				c = '\t'
			case 'r':
				c = '\r'
			}
			escape = false
		} else {
			switch c {
			case '\\':
				escape = true
				continue
			case '(':
				if cat {
					add_op('&')
				}
				fallthrough
			case '*', '|', ')':
				cat = c != '|' && c != '('
				add_op(c)
				continue
			}
		}
		if cat {
			add_op('&')
		}
		nfastack = append(nfastack, nfa_char(c))
		cat = true
	}
	for i := len(opstack) - 1; i >= 0; i-- {
		apply_op(opstack[i])
	}
	return nfastack[0]
}

func Match(re string, s string) bool {
	nfa := REtoNFA(re)
	nfa_alloc_id(nfa.Start, 1)
	nfa.End.accept = true
	dfa := nfa_to_dfa(nfa)
	return dfa.match(s)
}
