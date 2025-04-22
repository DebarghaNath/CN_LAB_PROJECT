# CN_LAB_PROJECT


Basis Idea:

1.Congestion Metric (CM) Definition
We define a Congestion Metric (CM) to evaluate the performance of a network topology or congestion control algorithm, incorporating key Quality of Service (QoS) parameters. The metric combines the impact of latency, throughput, and jitter to provide a comprehensive assessment of network congestion.
Parameters Considered:
Delay (D) – The average end-to-end packet delay.
Throughput (T) – The total amount of successfully delivered data per unit time.
Jitter (J) – The variation in packet delay, indicating delivery smoothness.
Metric Formula:
CM=α⋅D- β.T+γ⋅J
Where:
α, β, and γ are tunable weighting coefficients that reflect the relative importance of delay, throughput, and jitter respectively.
All parameters may be normalized based on predefined scale factors to ensure comparability.
2.Topologies Used for Experimentation
To evaluate the performance of congestion control algorithms under varying network structures, we utilize the following standard topologies. Each topology consists of 10 nodes, ensuring consistency in comparison:
 Linear
Mesh
Ring
Star
3.Objective: Minimization of the Congestion Metric
To enhance overall network performance, our primary goal is to reduce the Congestion Metric (CM), which is a function of key network parameters such as delay, throughput, and jitter.To achieve this, we introduce an adaptive strategy that dynamically adjusts the congestion window (cwnd). The change in congestion window is governed by the following equation:
Δcwnd=(1-PL)(a/(b+cwnd)) - PL(c*cwnd-d)
Where:
Δcwnd The incremental change to the congestion window.
PL The instantaneous packet‑loss probability, used to blend growth vs. reduction.
a,b Growth parameters:
	a controls the overall aggressiveness under low‑loss conditions
	b prevents unbounded increase when cWnd is small.
c,d Reduction parameters: 	c scales multiplicative decrease in response to loss 	d adds a constant decrement to avoid “train” behavior.
cwnd The current congestion‑window size (in bytes or segments).

Cases:
* Tahoe: a = 1,b = 0,c = 1,d = 1
	Δcwnd=(1/cwnd) - PL(1/(cwnd)+(cwnd-1))
* Reno: a = 1,b = 0,c = 1/2,d = 0
	Δcwnd=(1/cwnd) - PL((cwnd/2)+1/(cwnd))

4. Gradient‑Descent‑Driven Parameter Tuning After each measurement interval:
Compute the current Congestion Metric, CM =α.D - β.T +γ.J
Estimate each partial derivative via finite differences, e.g. ∂CM/∂x ≈ (CM(x+ε)−CM(x))/∂ε
Update parameters along the negative gradient: a←a−η∂CM/∂a ,b←b−η∂CM/∂b ,…where η η is the learning rate.
Convergence & Termination Iterate until the relative change in CM falls below a preset threshold, indicating that { {a,b,c,d} has converged to a near‑optimal operating point.
This procedure unifies online congestion control (via Δcwnd)  with offline optimization (via gradient descent), systematically driving the network toward a minimal congestion metric.
