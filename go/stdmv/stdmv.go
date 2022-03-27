//go:build go1.18

package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"go/parser"
	"go/token"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"regexp"
	"runtime"
	"strconv"
	"strings"
)

var ctx struct {
	GOROOT string
	GOOS   string
	GOARCH string
	GO1VER int
	GOTAGS map[string]bool

	StdDir  string
	BasePkg string
	SrcRoot string
	DstRoot string
	ModName string
	PkgName string
}

var (
	recursive bool

	distArch map[string]bool
	distOS   map[string]bool

	syncInternal bool
	depInternals = make(map[string]bool)

	newPkgName string
)

func main() {
	initCtx()

	files := flag.Args()
	if len(files) == 1 && isDir(filepath.Join(ctx.SrcRoot, files[0])) {
		copyPkgDir(filepath.Join(ctx.SrcRoot, files[0]), filepath.Join(ctx.DstRoot, files[0]), recursive)
	} else if len(files) > 0 {
		copyPkgFiles(ctx.SrcRoot, ctx.DstRoot, files)
	} else {
		flag.Usage()
		os.Exit(1)
	}

	if syncInternal {
		movedDeps := make(map[string]bool)
		for len(depInternals) > 0 {
			pending := make([]string, 0, len(depInternals))
			for pkgPath := range depInternals {
				if !movedDeps[pkgPath] {
					pending = append(pending, pkgPath)
				}
				delete(depInternals, pkgPath)
			}

			for _, pkgPath := range pending {
				mapped := mapInternalPkgPath(pkgPath)
				origPath := filepath.Join(ctx.StdDir, pkgPath)
				mappedPath := filepath.Join(ctx.DstRoot, mapped[len(ctx.ModName):])

				copyPkgDir(origPath, mappedPath, false)

				movedDeps[pkgPath] = true
			}
		}
	}
}

func copyPkgDir(srcDir string, dstDir string, rec bool) {
	debugf("copyPkgDir %s -> %s\n", srcDir, dstDir)

	entries, err := os.ReadDir(srcDir)
	stopOnErr("read dir", err)

	var (
		subDirs []string
		files   []string
	)
	for _, e := range entries {
		if e.IsDir() {
			subDirs = append(subDirs, e.Name())
			continue
		}

		name := e.Name()
		if !strings.HasSuffix(name, ".go") && !strings.HasSuffix(name, ".s") {
			continue
		}
		if strings.HasSuffix(name, "_test.go") {
			continue
		}
		distos, distarch := parseConstraintFromName(name)
		if (distos != "" && distos != ctx.GOOS) || (distarch != "" && distarch != ctx.GOARCH) {
			continue
		}
		files = append(files, name)
	}
	if len(files) > 0 {
		copyPkgFiles(srcDir, dstDir, files)
	}
	if !rec {
		return
	}
	for _, subDir := range subDirs {
		subSrcDir := filepath.Join(srcDir, subDir)
		subDstDir := filepath.Join(dstDir, subDir)
		copyPkgDir(subSrcDir, subDstDir, rec)
	}
}

func copyPkgFiles(srcDir string, dstDir string, files []string) {
	debugf("copyPkgFiles %s -> %s\n", srcDir, dstDir)

	if !isDir(dstDir) {
		stopOnErr("mkdir", os.MkdirAll(dstDir, 0755))
	}
	if dstDir == ctx.DstRoot {
		newPkgName = ctx.PkgName
	} else {
		newPkgName = ""
	}

	for _, file := range files {
		srcName := filepath.Join(srcDir, file)
		if !isRegularFile(srcName) {
			continue
		}
		dstName := filepath.Join(dstDir, file)
		if strings.HasSuffix(file, ".go") {
			stopOnErr("copy .go file", processGoFile(srcName, dstName))
		} else if strings.HasSuffix(file, ".s") {
			stopOnErr("copy .s file", copyAsmFile(srcName, dstName))
		} else {
			stopOnErr("copy file", copyFile(srcName, dstName))
		}
	}

	goFmt := exec.Command("gofmt", "-w", ".")
	goFmt.Stderr = os.Stderr
	goFmt.Dir = dstDir
	err := goFmt.Run()
	if err != nil {
		fmt.Fprintf(os.Stderr, "run gofmt: %v\n", err)
	}
}

func copyAsmFile(srcName string, dstName string) error {
	data, err := os.ReadFile(srcName)
	if err != nil {
		return err
	}
	var buildExpr string
	p := 0
	for p < len(data) {
		l := bytes.IndexByte(data[p:], '\n')
		if l < 0 {
			break
		}
		if l > 0 {
			if !bytes.HasPrefix(data[p:], []byte("//")) {
				break
			}
			if bytes.HasPrefix(data[p:], []byte("//go:build")) {
				buildExpr = string(bytes.TrimSpace(data[p+10 : p+l]))
			}
		}
		p = p + l + 1
	}
	if !evalGoBuildConstraint(buildExpr) {
		debugf("skip file: %s\n", srcName)
		return nil
	}

	debugf("process .s: %s -> %s\n", srcName, dstName)
	return os.WriteFile(dstName, data, 0644)
}

func processGoFile(srcName string, dstName string) error {
	srcData, err := os.ReadFile(srcName)
	if err != nil {
		return err
	}

	metadata, err := scanGoMetadata(srcData)
	if err != nil {
		return err
	}
	if !evalGoBuildConstraint(metadata.build) {
		debugf("skip file: %s\n", srcName)
		return nil
	}

	debugf("process .go: %s -> %s\n", srcName, dstName)

	content := [][]byte{srcData}
	if newPkgName != "" {
		content = shallowReplace(content,
			[]byte(fmt.Sprintf("package %s", metadata.pkgname)),
			[]byte(fmt.Sprintf("package %s", newPkgName)),
			1)
	}

	for _, pkgPath := range metadata.imports {
		orig := pkgPath[1 : len(pkgPath)-1]
		mapped := mapInternalPkgPath(orig)
		if mapped == orig {
			continue
		}
		depInternals[orig] = true
		debugf("map internal: %s -> %s\n", orig, mapped)
		content = shallowReplace(content,
			[]byte(pkgPath),
			[]byte("\""+mapped+"\""),
			1)
	}

	content = shallowReplace(content,
		[]byte("//go:"),
		func(next []byte) []byte {
			if bytes.HasPrefix(next, []byte("build")) {
				return []byte("//go:")
			} else {
				return []byte("//!go:")
			}
		},
		-1)

	f, err := os.Create(dstName)
	if err != nil {
		return err
	}
	defer f.Close()
	for _, seg := range content {
		_, err := f.Write(seg)
		if err != nil {
			return err
		}
	}
	return nil
}

func shallowReplace(segs [][]byte, find []byte, to any, n int) [][]byte {
	var sep func([]byte) []byte
	switch val := to.(type) {
	case []byte:
		sep = func(b []byte) []byte { return val }
	case func([]byte) []byte:
		sep = val
	default:
		panic("invalid `to` type")
	}
	return xflatn(n, segs, func(t []byte, m int) ([][]byte, int) {
		if m > 0 {
			m++
		}
		parts := bytes.SplitN(t, find, m)
		if len(parts) == 1 {
			return parts, 0
		}
		return xjoin(parts, sep), len(parts) - 1
	})
}

func parseConstraintFromName(name string) (goos string, goarch string) {
	ext := path.Ext(name)
	if ext == "" {
		return
	}
	segs := strings.Split(name[:len(name)-len(ext)], "_")
	n := len(segs)
	if n > 1 && distArch[segs[n-1]] {
		goarch = segs[n-1]
		n--
	}
	if n > 1 && distOS[segs[n-1]] {
		goos = segs[n-1]
	}
	return
}

func mapInternalPkgPath(pkgPath string) string {
	if pkgPath == "internal" || strings.HasPrefix(pkgPath, "internal/") {
		return path.Join(ctx.ModName, "internal/std", pkgPath[8:])
	}
	p := strings.Index(pkgPath, "/internal/")
	if p < 0 {
		if !strings.HasSuffix(pkgPath, "/internal") {
			return pkgPath
		}
		p = len(pkgPath) - 9
	}
	if strings.HasPrefix(pkgPath, ctx.BasePkg) {
		return path.Join(ctx.ModName, pkgPath[len(ctx.BasePkg):])
	}
	return path.Join(ctx.ModName, "internal/std", pkgPath[:p], pkgPath[p+9:])
}

type goMetadata struct {
	build   string
	pkgname string
	imports []string
}

func scanGoMetadata(src []byte) (*goMetadata, error) {
	f, err := parser.ParseFile(token.NewFileSet(), "", src, parser.ImportsOnly|parser.ParseComments)
	if err != nil {
		return nil, err
	}
	metadata := &goMetadata{
		pkgname: f.Name.Name,
	}
	for _, i := range f.Imports {
		metadata.imports = append(metadata.imports, i.Path.Value)
	}
	stop := false
	for _, cg := range f.Comments {
		if cg.Pos() > f.Name.Pos() {
			break
		}
		for _, c := range cg.List {
			if c.Text[1] == '*' {
				stop = true
			}
			if strings.HasPrefix(c.Text, "//go:build") {
				metadata.build = strings.TrimSpace(c.Text[10:])
				break
			}
			if strings.HasPrefix(strings.TrimSpace(c.Text[2:]), "+build") {
				fatalf("unsupport +build constraint syntax")
			}
		}
		if stop {
			break
		}
	}
	return metadata, nil
}

func isId(c byte) bool {
	return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || '0' <= c && c <= '9' || c == '.' || c == '_'
}

func checkGoBuildable(id string) bool {
	if ctx.GOOS == id || ctx.GOARCH == id || ctx.GOTAGS[id] {
		return true
	}
	if strings.HasPrefix(id, "go1.") {
		ver, _ := strconv.Atoi(id[4:])
		return ver <= ctx.GO1VER
	}
	return false
}

func evalGoBuildConstraint(expr string) bool {
	if expr == "" {
		return true
	}
	if expr == "ignore" {
		return false
	}
	bad := false
	i := 0
	var eval func() bool
	eval = func() bool {
		reg := false
		not := false
		binop := byte(0)
		for !bad {
			for i < len(expr) && (expr[i] == ' ' || expr[i] == '\t') {
				i++
			}
			if i >= len(expr) {
				break
			}
			c := expr[i]
			switch c {
			case '!':
				not = !not
				i++
			case '&', '|':
				if binop == 0 && !not {
					binop = c
					i += 2
				} else {
					bad = true
				}
			case ')':
				return reg
			default:
				var unary bool
				if c == '(' {
					i++
					unary = eval()
					if i < len(expr) && expr[i] == ')' {
						i++
					} else {
						bad = true
					}
				} else {
					b := i
					for i < len(expr) && isId(expr[i]) {
						i++
					}
					if b == i {
						bad = true
						return false
					}
					id := expr[b:i]
					if checkGoBuildable(id) {
						unary = true
					}
				}
				if not {
					unary = !unary
					not = false
				}
				switch binop {
				case '&':
					reg = reg && unary
				case '|':
					reg = reg || unary
				default:
					reg = unary
				}
				binop = 0
			}
		}
		return reg
	}

	res := eval()
	if bad {
		fmt.Fprintf(os.Stderr, "bad go:build constraint: %s\n", expr)
		return false
	}
	return res
}

func xjoin[T any](a []T, sep func(T) T) []T {
	r := make([]T, 0, len(a)+len(a)-1)
	for i, x := range a {
		if i > 0 {
			r = append(r, sep(x))
		}
		r = append(r, x)
	}
	return r
}

func xflatn[T any](n int, a []T, f func(T, int) ([]T, int)) []T {
	r := make([]T, 0, len(a))
	for i, x := range a {
		if n == 0 {
			r = append(r, a[i:]...)
			break
		}
		b, k := f(x, n)
		for _, j := range b {
			r = append(r, j)
		}
		if n > 0 {
			if n > k {
				n -= k
			} else {
				n = 0
			}
		}
	}
	return r
}

func copyFile(srcName string, dstName string) error {
	data, err := os.ReadFile(srcName)
	if err == nil {
		err = os.WriteFile(dstName, data, 0644)
	}
	return err
}

func isDir(path string) bool {
	fi, err := os.Stat(path)
	return err == nil && fi.IsDir()
}

func isRegularFile(path string) bool {
	fi, err := os.Stat(path)
	return err == nil && fi.Mode().IsRegular()
}

func initCtx() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "%s: [options] files...\n\n", filepath.Base(os.Args[0]))
		flag.PrintDefaults()
	}
	
	// TODO: load build config from go/build

	goVer, _ := strconv.Atoi(runtime.Version()[4:])

	var (
		gotags   string
		userDeps string
		basePkg  string
		dstRoot  string
		printCtx bool
	)
	flag.StringVar(&ctx.GOROOT, "goroot", runtime.GOROOT(), "GOROOT")
	flag.StringVar(&ctx.GOOS, "goos", runtime.GOOS, "GOOS")
	flag.StringVar(&ctx.GOARCH, "goarch", runtime.GOARCH, "GOARCH")
	flag.IntVar(&goVer, "gover", goVer, "go1 sub-version")
	flag.StringVar(&gotags, "gotags", "", "GOTAGS")
	flag.StringVar(&userDeps, "deps", "", "user deps")
	flag.StringVar(&basePkg, "base", "", "base package")
	flag.StringVar(&dstRoot, "d", ".", "destination directory")
	flag.StringVar(&ctx.PkgName, "pkgname", "", "destination package")
	flag.BoolVar(&recursive, "r", false, "recursive")
	flag.BoolVar(&syncInternal, "internal", false, "copy internal")
	flag.BoolVar(&printCtx, "printctx", false, "print ctx")
	flag.Parse()

	ctx.GO1VER = goVer
	ctx.GOTAGS = map[string]bool{
		"goexperiment.regabiwrappers": true,
		"goexperiment.regabireflect":  true,
		"goexperiment.regabiargs":     true,
		"goexperiment.pacerredesign":  true,
	}

	if gotags != "" {
		for _, tag := range strings.Split(gotags, ",") {
			if strings.HasPrefix(tag, "!") {
				delete(ctx.GOTAGS, tag[1:])
			} else {
				ctx.GOTAGS[tag] = true
			}
		}
	}

	if userDeps != "" {
		for _, dep := range strings.Split(userDeps, ",") {
			depInternals[dep] = true
		}
	}

	ctx.StdDir = filepath.Join(ctx.GOROOT, "src")
	ctx.BasePkg = basePkg
	if basePkg != "" {
		ctx.SrcRoot = filepath.Join(ctx.StdDir, basePkg)
	} else {
		ctx.SrcRoot = ctx.StdDir
	}
	ctx.DstRoot = filepath.Clean(dstRoot)
	if ctx.DstRoot != "." && !isDir(ctx.DstRoot) {
		stopOnErr("mkdir", os.MkdirAll(ctx.DstRoot, 0755))
	}

	var distData bytes.Buffer
	goDistList := exec.Command("go", "tool", "dist", "list", "-json")
	goDistList.Stderr = os.Stderr
	goDistList.Stdout = &distData
	stopOnErr("go tool dist list", goDistList.Run())

	var dists []struct {
		CgoSupported bool   `json:"CgoSupported"`
		FirstClass   bool   `json:"FirstClass"`
		GOARCH       string `json:"GOARCH"`
		GOOS         string `json:"GOOS"`
	}
	stopOnErr("parse go dist list", json.Unmarshal(distData.Bytes(), &dists))
	distArch = make(map[string]bool)
	distOS = make(map[string]bool)
	for _, dist := range dists {
		distArch[dist.GOARCH] = true
		distOS[dist.GOOS] = true
	}

	goModFile := filepath.Join(ctx.DstRoot, "go.mod")
	if !isRegularFile(goModFile) {
		goModInit := exec.Command("go", "mod", "init")
		goModInit.Stderr = os.Stderr
		goModInit.Dir = ctx.DstRoot
		stopOnErr("go mod init", goModInit.Run())
	}
	modName, err := loadGoModName(goModFile)
	stopOnErr("load go module name", err)
	ctx.ModName = modName

	if printCtx {
		data, _ := json.MarshalIndent(&ctx, "", "  ")
		fmt.Printf("context: %s\n", data)
	}
}

func loadGoModName(gomod string) (string, error) {
	data, err := os.ReadFile(gomod)
	if err != nil {
		return "", err
	}
	match := regexp.MustCompile(`module\s*(\S+)`).FindSubmatch(data)
	if len(match) == 0 {
		return "", fmt.Errorf("no module found")
	}
	return string(match[1]), err
}

func debugf(format string, args ...any) {
	fmt.Printf(format, args...)
}

func fatalf(format string, args ...any) {
	fmt.Fprintf(os.Stderr, format, args...)
	os.Exit(1)
}

func stopOnErr(op string, err error) {
	if err != nil {
		fatalf("%s: %v\n", op, err)
	}
}
