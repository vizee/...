package epaxos

import (
	"context"
	"encoding/base64"
	"fmt"
	"sort"
	"sync"
	"time"

	"github.com/vizee/echo"
	"github.com/vizee/litebuf"
	"github.com/vizee/litu/cmp"
	"github.com/vizee/litu/collections/vec"
	"github.com/vizee/litu/collections/vecdeque"
	"github.com/vizee/litu/slice"
)

const debugz = true

type CmdOp int

const (
	CmdOpNop CmdOp = iota
	CmdOpR
	CmdOpW
)

type InstanceState int

const (
	StateInit = iota
	StatePreAccepted
	StateAccepted
	StateCommitted
)

type Command struct {
	Op    CmdOp  `json:"op,omitempty"`
	Key   string `json:"key,omitempty"`
	Value []byte `json:"value,omitempty"`
}

func (v *Command) Echo(w *litebuf.Buffer) {
	w.WriteString("{op=")
	switch v.Op {
	case CmdOpNop:
		w.WriteString("nop")
	case CmdOpR:
		w.WriteString("r")
	case CmdOpW:
		w.WriteString("w")
	}
	if v.Key != "" {
		w.WriteString(" key=")
		w.WriteString(v.Key)
	}
	if len(v.Value) != 0 {
		w.WriteString(" value=")
		base64.StdEncoding.EncodeToString(v.Value)
	}
	w.WriteByte('}')
}

type BallotNumber struct {
	Epoch uint64 `json:"epoch,omitempty"`
	B     uint64 `json:"b,omitempty"`
	R     uint64 `json:"r,omitempty"`
}

func (v *BallotNumber) Echo(w *litebuf.Buffer) {
	w.WriteString("{epoch=")
	w.WriteUint(v.Epoch, 10)
	w.WriteString(" b=")
	w.WriteUint(v.B, 10)
	w.WriteString(" r=")
	w.WriteUint(v.R, 10)
	w.WriteByte('}')
}

type InstanceId struct {
	Rid   uint64 `json:"rid,omitempty"`
	Index uint64 `json:"index,omitempty"`
}

func (v *InstanceId) Echo(w *litebuf.Buffer) {
	w.WriteString("{rid=")
	w.WriteUint(v.Rid, 10)
	w.WriteString(" index=")
	w.WriteUint(v.Index, 10)
	w.WriteByte('}')
}

type Instance struct {
	I       *InstanceId   `json:"i,omitempty"`
	Ballot  *BallotNumber `json:"ballot,omitempty"`
	MaxSeen *BallotNumber `json:"-"`
	State   InstanceState `json:"state,omitempty"`
	Y       *Command      `json:"y,omitempty"`
	Seq     uint64        `json:"seq,omitempty"`
	Deps    []*InstanceId `json:"deps,omitempty"`
}

type PreAcceptReq struct {
	Ballot *BallotNumber `json:"ballot,omitempty"`
	Y      *Command      `json:"y,omitempty"`
	Seq    uint64        `json:"seq,omitempty"`
	Deps   []*InstanceId `json:"deps,omitempty"`
	I      *InstanceId   `json:"i,omitempty"`
}

func (v *PreAcceptReq) Echo(w *litebuf.Buffer) {
	w.WriteString("{ballot=")
	v.Ballot.Echo(w)
	w.WriteString(" y=")
	v.Y.Echo(w)
	w.WriteString(" seq=")
	w.WriteUint(v.Seq, 10)
	w.WriteString(" deps=")
	w.WriteByte('[')
	for i, dep := range v.Deps {
		if i > 0 {
			w.WriteByte(' ')
		}
		dep.Echo(w)
	}
	w.WriteByte(']')
	w.WriteString(" i=")
	v.I.Echo(w)
	w.WriteByte('}')
}

type PreAcceptResp struct {
	Ok     bool          `json:"ok,omitempty"`
	Ballot *BallotNumber `json:"ballot,omitempty"`
	Y      *Command      `json:"y,omitempty"`
	Seq    uint64        `json:"seq,omitempty"`
	Deps   []*InstanceId `json:"deps,omitempty"`
	I      *InstanceId   `json:"i,omitempty"`
}

type AcceptReq struct {
	Ballot *BallotNumber `json:"ballot,omitempty"`
	Y      *Command      `json:"y,omitempty"`
	Seq    uint64        `json:"seq,omitempty"`
	Deps   []*InstanceId `json:"deps,omitempty"`
	I      *InstanceId   `json:"i,omitempty"`
}

func (v *AcceptReq) Echo(w *litebuf.Buffer) {
	w.WriteString("{ballot=")
	v.Ballot.Echo(w)
	w.WriteString(" y=")
	v.Y.Echo(w)
	w.WriteString(" seq=")
	w.WriteUint(v.Seq, 10)
	w.WriteString(" deps=")
	w.WriteByte('[')
	for i, dep := range v.Deps {
		if i > 0 {
			w.WriteByte(' ')
		}
		dep.Echo(w)
	}
	w.WriteByte(']')
	w.WriteString(" i=")
	v.I.Echo(w)
	w.WriteByte('}')
}

type AcceptResp struct {
	Ok     bool          `json:"ok,omitempty"`
	Ballot *BallotNumber `json:"ballot,omitempty"`
	Y      *Command      `json:"y,omitempty"`
	I      *InstanceId   `json:"i,omitempty"`
}

type CommitReq struct {
	Ballot *BallotNumber `json:"ballot,omitempty"`
	Y      *Command      `json:"y,omitempty"`
	Seq    uint64        `json:"seq,omitempty"`
	Deps   []*InstanceId `json:"deps,omitempty"`
	I      *InstanceId   `json:"i,omitempty"`
}

func (v *CommitReq) Echo(w *litebuf.Buffer) {
	w.WriteString("{ballot=")
	v.Ballot.Echo(w)
	w.WriteString(" y=")
	v.Y.Echo(w)
	w.WriteString(" seq=")
	w.WriteUint(v.Seq, 10)
	w.WriteString(" deps=")
	w.WriteByte('[')
	for i, dep := range v.Deps {
		if i > 0 {
			w.WriteByte(' ')
		}
		dep.Echo(w)
	}
	w.WriteByte(']')
	w.WriteString(" i=")
	v.I.Echo(w)
	w.WriteByte('}')
}

type CommitResp struct {
}

type PrepareReq struct {
	Ballot *BallotNumber `json:"ballot,omitempty"`
	I      *InstanceId   `json:"i,omitempty"`
}

func (v *PrepareReq) Echo(w *litebuf.Buffer) {
	w.WriteString("{ballot=")
	v.Ballot.Echo(w)
	w.WriteString(" i=")
	v.I.Echo(w)
	w.WriteByte('}')
}

type PreparedCmd struct {
	Y     *Command      `json:"y,omitempty"`
	Seq   uint64        `json:"seq,omitempty"`
	Deps  []*InstanceId `json:"deps,omitempty"`
	State InstanceState `json:"state,omitempty"`
}

type PrepareResp struct {
	Ok     bool          `json:"ok,omitempty"`
	Ballot *BallotNumber `json:"ballot,omitempty"`
	I      *InstanceId   `json:"i,omitempty"`
	Cmd    *PreparedCmd  `json:"cmd,omitempty"`
}

type RequestReq struct {
	Y *Command `json:"y,omitempty"`
}

func (v *RequestReq) Echo(w *litebuf.Buffer) {
	w.WriteString("{y=")
	v.Y.Echo(w)
	w.WriteByte('}')
}

type RequestResp struct {
	Ok bool `json:"ok,omitempty"`
}

type ReadObjectReq struct {
	Key string `json:"key,omitempty"`
}

func (v *ReadObjectReq) Echo(w *litebuf.Buffer) {
	w.WriteString("{key=")
	w.WriteString(v.Key)
	w.WriteByte('}')
}

type ReadObjectResp struct {
	Ok    bool   `json:"ok,omitempty"`
	Value []byte `json:"value,omitempty"`
}

type EpaxosClient interface {
	PreAccept(ctx context.Context, in *PreAcceptReq) (*PreAcceptResp, error)
	Accept(ctx context.Context, in *AcceptReq) (*AcceptResp, error)
	Commit(ctx context.Context, in *CommitReq) (*CommitResp, error)
	Prepare(ctx context.Context, in *PrepareReq) (*PrepareResp, error)
	Request(ctx context.Context, in *RequestReq) (*RequestResp, error)
	ReadObject(ctx context.Context, in *ReadObjectReq) (*ReadObjectResp, error)
}

type Persist interface {
	Load(key string, v any) error
	Store(key string, v any) error
	List(prefix string) ([]string, error)
}

type Epaxos struct {
	rid   uint64         // 每个副本的 rid 都是固定且唯一的
	epoch uint64         // 每次更新配置 +1
	peers []EpaxosClient // 包含自己客户端列表
	state *State
	exe   *Executor
}

func (ep *Epaxos) PreAccept(ctx context.Context, req *PreAcceptReq) (*PreAcceptResp, error) {
	if debugz {
		echo.D("PreAccept", echo.Echo("req", req))
	}

	ep.state.lock.Lock()
	defer ep.state.lock.Unlock()
	curr := ep.state.getCmdLocked(req.I)
	if curr != nil {
		// TODO: 状态为 accepted 的 cmd 不应该被覆盖
		if lessBallotNumber(req.Ballot, curr.MaxSeen) || curr.State == StateCommitted {
			return &PreAcceptResp{
				Ok:     false,
				Ballot: curr.MaxSeen,
			}, nil
		}
	}

	maxSeq, cmdDeps, err := ep.state.getInterfLocked(req.Y.Key, req.Y.Op != CmdOpW)
	if err != nil {
		return nil, err
	}

	// TODO: 对比 curr.Deps 和 mergedDeps
	sortInstanceIds(cmdDeps)
	sortInstanceIds(req.Deps)
	mergedDeps := slice.UnionSortedBy(cmdDeps, req.Deps, cmpInstanceId)
	if req.Seq > maxSeq {
		maxSeq = req.Seq
	} else {
		maxSeq++
	}

	ins := &Instance{
		I:       req.I,
		Ballot:  req.Ballot,
		MaxSeen: req.Ballot,
		Y:       req.Y,
		Seq:     maxSeq,
		Deps:    mergedDeps,
		State:   StatePreAccepted,
	}

	err = ep.state.storeCmdLocked(ins)
	if err != nil {
		return nil, err
	}

	return &PreAcceptResp{
		Ok:     true,
		Ballot: ins.Ballot,
		Y:      ins.Y,
		Seq:    ins.Seq,
		Deps:   ins.Deps,
		I:      ins.I,
	}, nil
}

func (ep *Epaxos) Accept(ctx context.Context, req *AcceptReq) (*AcceptResp, error) {
	if debugz {
		echo.D("Accept", echo.Echo("req", req))
	}

	ep.state.lock.Lock()
	defer ep.state.lock.Unlock()
	curr := ep.state.getCmdLocked(req.I)
	if curr != nil {
		if lessBallotNumber(req.Ballot, curr.Ballot) || curr.State == StateCommitted {
			return &AcceptResp{
				Ok:     false,
				Ballot: curr.Ballot,
			}, nil
		}
	}

	ins := &Instance{
		I:       req.I,
		Ballot:  req.Ballot,
		MaxSeen: req.Ballot,
		Y:       req.Y,
		Seq:     req.Seq,
		Deps:    req.Deps,
		State:   StateAccepted,
	}

	err := ep.state.storeCmdLocked(ins)
	if err != nil {
		return nil, err
	}

	return &AcceptResp{
		Ok:     false,
		Ballot: ins.Ballot,
		Y:      ins.Y,
		I:      ins.I,
	}, nil
}

func (ep *Epaxos) Commit(ctx context.Context, req *CommitReq) (*CommitResp, error) {
	if debugz {
		echo.D("Commit", echo.Echo("req", req))
	}

	err := ep.state.storeCmd(&Instance{
		I:       req.I,
		Ballot:  req.Ballot,
		MaxSeen: req.Ballot,
		Y:       req.Y,
		Seq:     req.Seq,
		Deps:    req.Deps,
		State:   StateCommitted,
	})
	if err != nil {
		return nil, err
	}
	return &CommitResp{}, nil
}

func (ep *Epaxos) Prepare(ctx context.Context, req *PrepareReq) (*PrepareResp, error) {
	if debugz {
		echo.D("Prepare", echo.Echo("req", req))
	}

	ep.state.lock.Lock()
	defer ep.state.lock.Unlock()
	curr := ep.state.getCmdLocked(req.I)
	if curr != nil {
		if lessBallotNumber(req.Ballot, curr.Ballot) {
			return &PrepareResp{
				Ok:     false,
				Ballot: curr.Ballot,
			}, nil
		}
	} else {
		curr = &Instance{
			I:       req.I,
			Ballot:  &BallotNumber{},
			MaxSeen: req.Ballot,
			State:   StateInit,
			Y: &Command{
				Op: CmdOpNop,
			},
		}
	}

	return &PrepareResp{
		Ok:     true,
		Ballot: curr.Ballot,
		I:      curr.I,
		Cmd: &PreparedCmd{
			Y:     curr.Y,
			Seq:   curr.Seq,
			Deps:  curr.Deps,
			State: curr.State,
		},
	}, nil
}

func (ep *Epaxos) Request(ctx context.Context, req *RequestReq) (*RequestResp, error) {
	if debugz {
		echo.D("Request", echo.Echo("req", req))
	}

	ins, err := ep.commitCmd(ctx, req.Y)
	if err != nil {
		return nil, err
	}
	if ins == nil {
		return &RequestResp{Ok: false}, nil
	}

	ep.executeAsync(ins, nil)

	return &RequestResp{Ok: true}, nil
}

func (ep *Epaxos) ReadObject(ctx context.Context, req *ReadObjectReq) (*ReadObjectResp, error) {
	if debugz {
		echo.D("ReadObject", echo.Echo("req", req))
	}

	ins, err := ep.commitCmd(ctx, &Command{
		Op:  CmdOpR,
		Key: req.Key,
	})
	if err != nil {
		return nil, err
	}
	if ins == nil {
		return &ReadObjectResp{
			Ok: false,
		}, nil
	}

	// NOTE: 如果 ins 因为 recovery 提前被执行，可能永远收不到结果
	res := make(chan []byte, 1)
	if ep.executeAsync(ins, res) {
		select {
		case val := <-res:
			return &ReadObjectResp{
				Ok:    true,
				Value: val,
			}, nil
		case <-ctx.Done():
			ep.exe.removeCmdRes(ins.I, res)
			err := ctx.Err()
			if err != nil {
				return nil, err
			}
		}
	}

	return &ReadObjectResp{
		Ok: false,
	}, nil
}

func NewEpaxos(rid uint64, epoch uint64, peers []EpaxosClient, state *State, exec *Executor) *Epaxos {
	return &Epaxos{
		rid:   rid,
		epoch: epoch,
		peers: peers,
		state: state,
		exe:   exec,
	}
}

type invokeResult[R any] struct {
	who int
	r   R
	e   error
}

func invokeAsync[R any](ep *Epaxos, invoke func(peer EpaxosClient) (R, error)) (chan invokeResult[R], int) {
	results := make(chan invokeResult[R], len(ep.peers))
	total := 0
	for i, peer := range ep.peers {
		if ep.rid == uint64(i) {
			continue
		}

		if peer == nil {
			continue
		}

		total++
		go func(who int, peer EpaxosClient) {
			const maxRetry = 3
			retry := 0
			for {
				r, err := invoke(peer)
				if err != nil && retry < maxRetry {
					retry++
					time.Sleep(time.Duration(1<<retry) * 10 * time.Millisecond)
					continue
				}
				results <- invokeResult[R]{who: who, r: r, e: err}
				return
			}
		}(i, peer)
	}
	return results, total
}

func (ep *Epaxos) phase1(ctx context.Context, ins *Instance) (bool, error) {
	if debugz {
		echo.D("pre-accept phase", echo.Echo("i", ins.I))
	}

	err := ep.state.storeCmd(ins)
	if err != nil {
		return false, err
	}

	// 不可以直接用 ins，否则会有竞争问题
	req := &PreAcceptReq{
		Ballot: ins.Ballot,
		Y:      ins.Y,
		Seq:    ins.Seq,
		Deps:   ins.Deps,
		I:      ins.I,
	}
	// PreAccept 和 Accept 可以通过 thrifty operation 完成，失败时合并返回ok的副本和未参与的副本重试
	results, total := invokeAsync(ep, func(peer EpaxosClient) (*PreAcceptResp, error) { return peer.PreAccept(ctx, req) })

	// 每个副本收到 PreAccept 消息后，根据自身的 log 内容更新 γ 的 deps 和 seq，在 log 里记录 γ 和更新后的属性。

	sortInstanceIds(ins.Deps)
	mergedDeps := ins.Deps
	maxSeq := ins.Seq
	isDiff := false
	majorityQuorum := len(ep.peers)/2 + 1
	f := len(ep.peers) - majorityQuorum
	fastPathQuorum := f + (f+1)/2
	preAccepted := 1 // 副本自己已经算成一票
	for i := 0; i < total; i++ {
		res := <-results
		if res.e != nil {
			echo.E("PreAccept error", echo.Int("peer", res.who), echo.Errors(res.e))
			continue
		}
		resp := res.r
		if !resp.Ok {
			if lessBallotNumber(ins.MaxSeen, resp.Ballot) {
				ins.MaxSeen = resp.Ballot
			}
			echo.I("PreAccept error", echo.Int("peer", res.who), echo.Echo("i", ins.I))
			continue
		}

		// 如果命令领导者从副本收到回复满足 fast-path quorum，并且所有更新的属性都相同，则认为命令可提交。
		// 如果没有收到足够的回复或者某些回复中属性和其他回复属性不同,命令领导者会根据 majority 回复的所有 deps 的并集和最高的 seq 更新属性
		// 并且告知 majority 副本 Accept 这些属性，然后认为命令可提交。
		sortInstanceIds(resp.Deps)
		if !slice.Equal(ins.Deps, resp.Deps, func(a, b *InstanceId) bool { return cmpInstanceId(a, b) == 0 }) {
			isDiff = true
			mergedDeps = slice.UnionSortedBy(mergedDeps, resp.Deps, cmpInstanceId)
			if resp.Seq > maxSeq {
				maxSeq = resp.Seq
			}
		}

		preAccepted++
		if preAccepted >= fastPathQuorum || (isDiff && preAccepted >= majorityQuorum) {
			break
		}
	}

	// 优化：在时间窗口内，如果 fast-quorum 满足 deps 相同，则尽量不使用 merged deps
	if isDiff {
		ins.Deps = mergedDeps
		ins.Seq = maxSeq
	}

	if preAccepted >= fastPathQuorum && !isDiff {
		err := ep.commitPhase(ctx, ins)
		if err != nil {
			return false, err
		}
		return true, nil
	}

	if preAccepted < majorityQuorum {
		echo.I("not enough replicas pre-accepts command", echo.Echo("i", ins.I))
		return false, nil
	}

	return ep.paxosAcceptPhase(ctx, ins)
}

func (ep *Epaxos) paxosAcceptPhase(ctx context.Context, ins *Instance) (bool, error) {
	if debugz {
		echo.D("accept phase", echo.Echo("i", ins.I))
	}

	ins.State = StateAccepted
	err := ep.state.storeCmd(ins)
	if err != nil {
		return false, err
	}

	req := &AcceptReq{
		Ballot: ins.Ballot,
		Y:      ins.Y,
		Seq:    ins.Seq,
		Deps:   ins.Deps,
		I:      ins.I,
	}
	results, total := invokeAsync(ep, func(peer EpaxosClient) (*AcceptResp, error) { return peer.Accept(ctx, req) })

	majorityQuorum := len(ep.peers)/2 + 1
	accepted := 1
	for i := 0; i < total; i++ {
		res := <-results
		if res.e != nil {
			echo.E("Accept error", echo.Int("peer", res.who), echo.Errors(res.e))
			continue
		}
		resp := res.r
		if !resp.Ok {
			if lessBallotNumber(ins.MaxSeen, resp.Ballot) {
				ins.MaxSeen = resp.Ballot
			}
			echo.I("Accept error", echo.Int("peer", res.who), echo.Echo("i", ins.I))
			continue
		}

		accepted++
		if accepted >= majorityQuorum {
			break
		}
	}

	if accepted < majorityQuorum {
		echo.I("not enough replicas accepts command", echo.Echo("i", ins.I))
		return false, nil
	}

	err = ep.commitPhase(ctx, ins)
	if err != nil {
		return false, err
	}
	return true, nil
}

func (ep *Epaxos) commitPhase(ctx context.Context, ins *Instance) error {
	if debugz {
		echo.D("commit phase", echo.Echo("i", ins.I))
	}

	// 认为命令可提交以后命令领导者回复客户端，并且向其他副本异步发送 Commit 消息。
	ins.State = StateCommitted
	err := ep.state.storeCmd(ins)
	if err != nil {
		return err
	}

	// 这里异步把 ins 传给 commitCommand，注意后续不能再次修改对象，否则出现竞争
	go func() {
		req := &CommitReq{
			Ballot: ins.Ballot,
			Y:      ins.Y,
			Seq:    ins.Seq,
			Deps:   ins.Deps,
			I:      ins.I,
		}
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		results, total := invokeAsync(ep, func(peer EpaxosClient) (*CommitResp, error) { return peer.Commit(ctx, req) })
		for i := 0; i < total; i++ {
			res := <-results
			if res.e != nil {
				echo.E("Commit error", echo.Int("peer", res.who), echo.Errors(res.e))
				continue
			}

			echo.I("Commit ok", echo.Int("peer", res.who), echo.Echo("I", ins.I))
		}
	}()

	return nil
}

func (ep *Epaxos) commitCmd(ctx context.Context, y *Command) (*Instance, error) {
	if debugz {
		echo.D("commit command", echo.Echo("y", y))
	}

	// 副本 L 提交命令 γ 流程：
	// L 在自己的实例子空间(sub-space)中选择下一个可用实例
	instaceIdx, err := ep.state.allocInstance()
	if err != nil {
		return nil, err
	}

	if debugz {
		echo.D("allocate instance", echo.Uint64("idx", instaceIdx))
	}

	// 从自身收集到的属性作为初始属性
	// deps：干扰 γ 的命令的实例列表（不一定已提交）。
	// seq：执行算法用来解决依赖循环的序列号，seq 设为大于 deps 中所有干扰命令的 seq。
	maxSeq, cmdDeps, err := ep.state.getInterf(y.Key, y.Op != CmdOpW)
	if err != nil {
		return nil, err
	}

	ballot := &BallotNumber{
		Epoch: ep.epoch,
		B:     0,
		R:     ep.rid,
	}
	ins := &Instance{
		I: &InstanceId{
			Rid:   ep.rid,
			Index: instaceIdx,
		},
		Ballot:  ballot,
		MaxSeen: ballot,
		Y:       y,
		Seq:     maxSeq + 1,
		Deps:    cmdDeps,
		State:   StatePreAccepted,
	}

	// 命令领导者把命令和初始属性作为 PreAccept 消息发送给至少一个 fast-path quorum 组的副本。
	ok, err := ep.phase1(ctx, ins)
	if err != nil {
		return nil, err
	}
	if !ok {
		return nil, nil
	}

	echo.I("command committed", echo.Echo("i", ins.I))

	return ins, nil
}

func (ep *Epaxos) explicitPrepare(ctx context.Context, ins *Instance) (bool, error) {
	nextBallot := &BallotNumber{
		Epoch: ep.epoch,
		B:     ins.MaxSeen.B + 1,
		R:     ep.rid,
	}

	if debugz {
		echo.D("explicit prepare", echo.Echo("i", ins.I), echo.Echo("next_ballot", nextBallot))
	}

	req := &PrepareReq{
		Ballot: nextBallot,
		I:      ins.I,
	}
	results, total := invokeAsync(ep, func(peer EpaxosClient) (*PrepareResp, error) { return peer.Prepare(ctx, req) })

	go func() {
		resp, err := ep.Prepare(ctx, req)
		results <- invokeResult[*PrepareResp]{
			who: int(ep.rid),
			r:   resp,
			e:   err,
		}
	}()
	total++

	n := len(ep.peers)
	majorityQuorum := n/2 + 1
	prepared := 0
	highestBallot := &BallotNumber{
		Epoch: 0,
		B:     0,
		R:     0,
	}
	// let R be the set of replies w/ the highest ballot number
	var r []*PrepareResp
	for i := 0; i < total; i++ {
		res := <-results
		if res.e != nil {
			echo.E("Prepare error", echo.Int("peer", res.who), echo.Errors(res.e))
			continue
		}
		resp := res.r
		if !resp.Ok {
			if lessBallotNumber(ins.MaxSeen, resp.Ballot) {
				ins.MaxSeen = resp.Ballot
			}
			echo.I("Prepare denied", echo.Int("peer", res.who), echo.Echo("curr", resp.Ballot))
			continue
		}

		if !lessBallotNumber(highestBallot, resp.Ballot) {
			if !equalBallotNumber(highestBallot, resp.Ballot) {
				highestBallot = resp.Ballot
				r = nil
			}
			r = append(r, resp)
		}

		prepared++
		// NOTE: len(results) 并不能很好感知已到达消息，应该使用 select w/ default，最好方式是设置一个时间窗口，收取窗口内尽可能多的 results
		if prepared >= majorityQuorum && len(results) == 0 {
			break
		}
	}

	if debugz {
		echo.D("prepared highest ballot", echo.Echo("i", ins.I), echo.Echo("highest_ballot", highestBallot), echo.Int("R", len(r)))
	}

	// if R contains a (γ, seqγ , depsγ , committed) then
	//   run Commit phase for (γ, seqγ , depsγ ) at L.i
	// else if R contains an (γ, seqγ , depsγ , accepted) then
	//   run Paxos-Accept phase for (γ, seqγ , depsγ ) at L.i
	// else if R contains at least (N/2) identical replies (γ, seqγ , depsγ , pre-accepted) for the default ballot epoch.0.L of instance L.i, and none of those replies is from L then
	//   + looks for only (F+1)/2 replicas that have pre-accepted a tuple (γ, depsγ , seqγ ) in the current instance with identical attributes
	//   + convince other replicas to pre-accept this tuple by sending TryPreAccept messages
	//   run Paxos-Accept phase for (γ, seqγ , depsγ ) at L.i
	// else if R contains at least one (γ, seqγ , depsγ , pre-accepted) then
	//   start Phase 1 (at line 2) for γ at L.i, avoid fast path
	// else
	//   start Phase 1 (at line 2) for no-op at L.i, avoid fast path

	if pr := slice.FirstPtr(r, func(pr *PrepareResp) bool { return pr.Cmd.State == StateCommitted }); pr != nil {
		err := ep.commitPhase(ctx, &Instance{
			I:       ins.I,
			Ballot:  nextBallot,
			MaxSeen: nextBallot,
			State:   pr.Cmd.State,
			Y:       pr.Cmd.Y,
			Seq:     pr.Cmd.Seq,
			Deps:    pr.Cmd.Deps,
		})
		if err != nil {
			return false, err
		}
		return true, nil
	} else if pr := slice.FirstPtr(r, func(pr *PrepareResp) bool { return pr.Cmd.State == StateAccepted }); pr != nil {
		return ep.paxosAcceptPhase(ctx, &Instance{
			I:       ins.I,
			Ballot:  nextBallot,
			MaxSeen: nextBallot,
			State:   pr.Cmd.State,
			Y:       pr.Cmd.Y,
			Seq:     pr.Cmd.Seq,
			Deps:    pr.Cmd.Deps,
		})
	} else if slice.CountBy(r, func(pr *PrepareResp) bool { return pr.Cmd.State == StatePreAccepted && pr.Ballot.B == 0 }) >= majorityQuorum {
		pr := slice.FirstPtr(r, func(pr *PrepareResp) bool { return pr.Cmd.State == StatePreAccepted && pr.Ballot.B == 0 })
		return ep.paxosAcceptPhase(ctx, &Instance{
			I:       ins.I,
			Ballot:  nextBallot,
			MaxSeen: nextBallot,
			State:   pr.Cmd.State,
			Y:       pr.Cmd.Y,
			Seq:     pr.Cmd.Seq,
			Deps:    pr.Cmd.Deps,
		})
	} else if pr := slice.FirstPtr(r, func(pr *PrepareResp) bool { return pr.Cmd.State == StatePreAccepted }); pr != nil {
		return ep.phase1(ctx, &Instance{
			I:       ins.I,
			Ballot:  nextBallot,
			MaxSeen: nextBallot,
			State:   pr.Cmd.State,
			Y:       pr.Cmd.Y,
			Seq:     pr.Cmd.Seq,
			Deps:    pr.Cmd.Deps,
		})
	} else {
		return ep.phase1(ctx, &Instance{
			I:       ins.I,
			Ballot:  nextBallot,
			MaxSeen: nextBallot,
			State:   StatePreAccepted,
			Y: &Command{
				Op: CmdOpNop,
			},
		})
	}
}

func (ep *Epaxos) recoveryDeps(ctx context.Context, root *Instance, lastSeq uint64) (bool, error) {
	q := vecdeque.New[*Instance](10)
	q.PushBack(root)
	for {
		v := q.PopFront()
		if !v.IsSome() {
			break
		}

		ins := v.Inner()
		if ins.Seq <= lastSeq {
			continue
		}
		for _, depId := range ins.Deps {
			depIns := ep.state.getCmd(depId)
			if depIns == nil || depIns.State != StateCommitted {
				if depIns == nil {
					depIns = &Instance{
						I:       depId,
						MaxSeen: &BallotNumber{},
					}
				} else {
					depIns = cloneInstance(depIns)
				}
				ok, err := ep.explicitPrepare(ctx, depIns)
				if err != nil || !ok {
					return false, err
				}
				depIns = ep.state.getCmd(depId)
			}
			if depIns.Seq < ins.Seq {
				q.PushBack(depIns)
			}
		}
	}
	return true, nil
}

type eacmd struct {
	ins   *Instance
	deps  []*eacmd
	dfn   int
	low   int
	instk bool
}

type eacontext struct {
	vis    map[InstanceId]*eacmd
	dfncnt int
	stack  []*eacmd
	res    []*eacmd
}

func (ep *Epaxos) collectCmds(ctx *eacontext, root *Instance) *eacmd {
	cmd := &eacmd{
		ins: root,
	}
	ctx.vis[*root.I] = cmd
	for _, depId := range root.Deps {
		depCmd := ctx.vis[*depId]
		if depCmd == nil {
			depCmd = ep.collectCmds(ctx, ep.state.getCmdLocked(depId))
			ctx.vis[*depId] = depCmd
		}
		cmd.deps = append(cmd.deps, depCmd)
	}
	return cmd
}

func (ep *Epaxos) resolveScc(ctx *eacontext, cmd *eacmd) {
	ctx.dfncnt++
	cmd.dfn = ctx.dfncnt
	cmd.low = ctx.dfncnt
	cmd.instk = true
	ctx.stack = append(ctx.stack, cmd)
	for _, dep := range cmd.deps {
		if dep.dfn == 0 {
			ep.resolveScc(ctx, dep)
			cmd.low = cmp.Min(cmd.low, dep.low)
		} else if dep.instk {
			cmd.low = cmp.Min(cmd.low, dep.dfn)
		}
	}
	if cmd.dfn == cmd.low {
		n := len(ctx.stack) - 1
		for ctx.stack[n] != cmd {
			ctx.stack[n].instk = false
			n--
		}
		ctx.stack[n].instk = false
		scc := ctx.stack[n:]
		ctx.stack = ctx.stack[:n]
		if len(scc) > 1 {
			sort.Slice(scc, func(i, j int) bool { return scc[i].ins.Seq > scc[j].ins.Seq })
		}
		ctx.res = append(ctx.res, scc...)
	}
}

func (ep *Epaxos) executeCmd(ctx context.Context, root *Instance) (bool, error) {
	if debugz {
		echo.D("execute command", echo.Echo("i", root.I))
	}

	lastSeq := uint64(0)
	ep.exe.lock.Lock()
	vm := ep.exe.kv[root.Y.Key]
	if vm != nil {
		lastSeq = vm.seq
	}
	ep.exe.lock.Unlock()

	// 副本在 R.i 实例执行命令 γ 流程：
	// 1.等待 i 提交
	ok, err := ep.recoveryDeps(ctx, root, lastSeq)
	if err != nil || !ok {
		return false, err
	}

	// 2.把 γ 和 deps 的命令作为节点构建 γ 的依赖图，从 γ 所在节点开始对有向边命令递归重复执行命令流程
	eactx := &eacontext{
		vis: make(map[InstanceId]*eacmd),
	}
	ep.state.lock.Lock()
	earoot := ep.collectCmds(eactx, root)
	// 3.找到所有强连通分量，拓扑排序
	// 按拓扑排序倒序，对每个强连通分量处理：
	// 1.按序列号排序强连通分量里的所有命令
	// 2.按序列号增序执行未执行的命令并标记命令已执行
	ep.state.lock.Unlock()
	ep.resolveScc(eactx, earoot)

	for i := len(eactx.res) - 1; i >= 0; i-- {
		cmd := eactx.res[i].ins
		if debugz {
			echo.D("apply command", echo.Echo("i", cmd.I))
		}
		ep.exe.applyCmd(cmd)
	}

	return true, nil
}

func (ep *Epaxos) executeAsync(ins *Instance, res chan []byte) bool {
	echo.I("enqueue command", echo.Echo("i", ins.I))
	return ep.exe.addCmd(ins.Y.Key, ins.Seq, ins.I, res)
}

func (ep *Epaxos) RunStep(ctx context.Context) (bool, error) {
	id, err := ep.exe.pullCmd(ctx)
	if err != nil {
		return false, err
	}
	// 从 exec 中 pull 的命令必然已 committed
	ins := ep.state.getCmd(id)
	return ep.executeCmd(ctx, ins)
}

type valueMeta struct {
	data []byte
	seq  uint64
}

type Executor struct {
	sig  chan struct{}
	kv   map[string]*valueMeta
	read map[InstanceId]*vec.Vec[chan []byte]
	q    *vecdeque.VecDeque[*InstanceId]
	lock sync.Mutex
}

func (e *Executor) pullCmd(ctx context.Context) (*InstanceId, error) {
	e.lock.Lock()
	for {
		v := e.q.PopFront()
		e.lock.Unlock()
		if v.IsSome() {
			return v.Inner(), nil
		}
		select {
		case <-e.sig:
		case <-ctx.Done():
			return nil, ctx.Err()
		}
		e.lock.Lock()
	}
}

func (e *Executor) addCmd(key string, seq uint64, i *InstanceId, res chan []byte) bool {
	e.lock.Lock()
	defer e.lock.Unlock()

	if vm := e.kv[key]; vm != nil && vm.seq >= seq {
		return false
	}

	e.q.PushBack(i)

	if res != nil {
		rs := e.read[*i]
		if rs == nil {
			rs = &vec.Vec[chan []byte]{}
			e.read[*i] = rs
		}
		rs.Push(res)
	}

	select {
	case e.sig <- struct{}{}:
	default:
	}

	return true
}

func (e *Executor) removeCmdRes(i *InstanceId, res chan []byte) {
	e.lock.Lock()
	defer e.lock.Unlock()

	rs := e.read[*i]
	if rs != nil {
		pos := rs.FindBy(&res, func(a, b *chan []byte) bool { return *a == *b })
		if pos.IsSome() {
			rs.SwapRemove(int(pos))
		}
	}
}

func (e *Executor) applyCmd(ins *Instance) {
	e.lock.Lock()
	vm := e.kv[ins.Y.Key]
	if ins.Y.Op == CmdOpW {
		if vm == nil {
			vm = &valueMeta{}
			e.kv[ins.Y.Key] = vm
		}
		vm.data = ins.Y.Value
		vm.seq = ins.Seq
	}
	rs := e.read[*ins.I]
	if rs != nil {
		delete(e.read, *ins.I)
	}
	var data []byte
	if vm != nil {
		data = vm.data
	}
	e.lock.Unlock()

	if data != nil && rs != nil {
		for _, ch := range rs.Slice() {
			ch <- data
		}
	}
}

func NewExecutor() *Executor {
	return &Executor{
		sig:  make(chan struct{}, 1),
		kv:   make(map[string]*valueMeta),
		read: make(map[InstanceId]*vec.Vec[chan []byte]),
		q:    vecdeque.New[*InstanceId](16),
	}
}

type keyDep struct {
	lastSeq uint64
	lastId  *InstanceId
	wSeq    uint64
	wId     *InstanceId
}

type State struct {
	persist Persist

	cmds [][]*Instance
	root map[string]*keyDep

	instanceIdx uint64 // 每次分配 instance +1

	lock sync.Mutex
}

func (s *State) getCmdLocked(id *InstanceId) *Instance {
	if id.Rid < uint64(len(s.cmds)) && id.Index < uint64(len(s.cmds[id.Rid])) {
		return s.cmds[id.Rid][id.Index]
	}
	return nil
}

func (s *State) getCmd(id *InstanceId) *Instance {
	s.lock.Lock()
	defer s.lock.Unlock()
	return s.getCmdLocked(id)
}

func (s *State) setCmdLocked(ins *Instance) {
	if ins.I.Rid >= uint64(len(s.cmds)) {
		t := make([][]*Instance, ins.I.Rid)
		copy(t, s.cmds)
		s.cmds = t
	}
	cmdsR := s.cmds[ins.I.Rid]
	if ins.I.Index >= uint64(len(cmdsR)) {
		t := make([]*Instance, ins.I.Index+1)
		copy(t, cmdsR)
		s.cmds[ins.I.Rid] = t
		cmdsR = t
	}
	cmdsR[ins.I.Index] = ins

	dep := s.root[ins.Y.Key]
	if dep == nil {
		dep = &keyDep{}
		s.root[ins.Y.Key] = dep
	}
	if dep.lastSeq < ins.Seq {
		dep.lastSeq = ins.Seq
		dep.lastId = ins.I
	}
	if ins.Y.Op == CmdOpW && dep.wSeq < ins.Seq {
		dep.wSeq = ins.Seq
		dep.wId = ins.I
	}
}

func (s *State) storeCmdLocked(ins *Instance) error {
	c := cloneInstance(ins)
	err := s.persist.Store(fmt.Sprintf("/cmds/%d/%d", c.I.Rid, c.I.Index), c)
	if err != nil {
		return err
	}

	s.setCmdLocked(c)

	return nil
}

func (s *State) storeCmd(ins *Instance) error {
	s.lock.Lock()
	defer s.lock.Unlock()
	return s.storeCmdLocked(ins)
}

func (s *State) loadCmdsLocked() error {
	keys, err := s.persist.List("/cmds/")
	if err != nil {
		return err
	}

	s.cmds = [][]*Instance{}
	s.root = make(map[string]*keyDep)
	for _, key := range keys {
		ins := new(Instance)
		err := s.persist.Load(key, ins)
		if err != nil {
			return err
		}
		ins.MaxSeen = ins.Ballot

		s.setCmdLocked(ins)
	}

	return nil
}

func (s *State) allocInstance() (uint64, error) {
	s.lock.Lock()
	defer s.lock.Unlock()

	next := s.instanceIdx + 1
	err := s.persist.Store("/instance_idx", next)
	if err != nil {
		return 0, err
	}
	s.instanceIdx = next
	return next, nil
}

func (s *State) getInterfLocked(key string, ro bool) (uint64, []*InstanceId, error) {
	dep := s.root[key]
	if dep != nil {
		if ro && dep.wSeq != 0 {
			return dep.lastSeq, []*InstanceId{dep.lastId}, nil
		} else {
			return dep.lastSeq, []*InstanceId{dep.lastId}, nil
		}
	}
	return 0, nil, nil
}

func (s *State) getInterf(key string, ro bool) (uint64, []*InstanceId, error) {
	s.lock.Lock()
	defer s.lock.Unlock()
	return s.getInterfLocked(key, ro)
}

func LoadState(persist Persist) (*State, error) {
	var instanceIdx uint64
	err := persist.Load("/instance_idx", &instanceIdx)
	if err != nil {
		return nil, err
	}

	s := &State{
		persist:     persist,
		instanceIdx: instanceIdx,
	}
	err = s.loadCmdsLocked()
	if err != nil {
		return nil, err
	}

	return s, nil
}

func cmpInstanceId(a *InstanceId, b *InstanceId) int {
	if a.Rid == b.Rid {
		return int(a.Index - b.Index)
	} else {
		return int(a.Rid - b.Rid)
	}
}

func sortInstanceIds(c []*InstanceId) {
	sort.Slice(c, func(i, j int) bool { return cmpInstanceId(c[i], c[j]) < 0 })
}

func lessBallotNumber(a *BallotNumber, b *BallotNumber) bool {
	if a.Epoch == b.Epoch {
		if a.B == b.B {
			return a.R < b.R
		}
		return a.B < b.B
	}
	return a.Epoch < b.Epoch
}

func equalBallotNumber(a *BallotNumber, b *BallotNumber) bool {
	return a.Epoch == b.Epoch && a.B == b.B && a.R == b.R
}

func cloneInstance(ins *Instance) *Instance {
	return &Instance{
		I:       ins.I,
		Ballot:  ins.Ballot,
		MaxSeen: ins.MaxSeen,
		Y:       ins.Y,
		Seq:     ins.Seq,
		Deps:    ins.Deps,
		State:   ins.State,
	}
}
