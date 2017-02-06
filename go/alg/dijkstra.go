package alg

// dijkstra
// g: 图邻接矩阵, 0代表没边
// s: 起点顶点
// e: 终点顶点
// 返回: 最短路径, -1代表没有路径
func dijkstra(g [][]int, s int, e int) int {
	// 图大小
	n := len(g)
	// s到所有顶点的距离
	d := make([]int, n)
	// 标记已经访问过的顶点
	f := make([]bool, n)
	for i := 0; i < n; i++ {
		d[i] = -1
	}
	d[s] = 0
	current := s
	for current != e {
		// 标记当前顶点访问过
		f[current] = true
		// 遍历所有和当前相连的边
		for i := 0; i < n; i++ {
			if g[current][i] == 0 {
				continue
			}
			// 如果没有到i顶点的距离, 或者当前顶点到i顶点的距离短于其他顶点到i顶点的距离
			if d[i] == -1 || d[current]+g[current][i] < d[i] {
				// 更新到i顶点的距离
				d[i] = d[current] + g[current][i]
			}
		}
		// 寻找下一个顶点
		next := -1
		for i := 0; i < n; i++ {
			// 过滤走过的点
			if f[i] {
				continue
			}
			// 找到下一条路径最短的没有访问过的顶点
			if d[i] != -1 && (next == -1 || d[i] < next) {
				next = d[i]
				current = i
			}
		}
		// 没有下一个顶点了
		if next == -1 {
			break
		}
	}
	return d[e]
}
