# CN_LAB_PROJECT

## 📌 Overview

This project investigates **congestion control algorithms** and their performance in various network topologies using a custom-defined **Congestion Metric (CM)**. The goal is to **minimize congestion** through adaptive control mechanisms and gradient-descent-based parameter tuning.

---

## 💡 1. Congestion Metric (CM) Definition

We define a **Congestion Metric (CM)** to evaluate the performance of a network topology or congestion control algorithm by incorporating key **Quality of Service (QoS)** parameters.

### 📊 Parameters Considered
- **Delay (D)** – Average end-to-end packet delay (in ms)
- **Throughput (T)** – Total amount of successfully delivered data per unit time (in Mbps)
- **Jitter (J)** – Variation in packet delay (in ms), indicating delivery smoothness

### 🧮 Metric Formula

CM = α · D - β · T + γ · J


Where:
- **α**, **β**, and **γ** are tunable weighting coefficients that reflect the relative importance of **delay**, **throughput**, and **jitter** respectively.
- All parameters may be **normalized** based on predefined scale factors to ensure comparability.

---

## 🌐 2. Topologies Used for Experimentation

To evaluate congestion control performance, we experiment on **four standard network topologies**, each consisting of **10 nodes**:

- 🔗 **Linear** – Nodes connected in a single chain  
- 🌐 **Mesh** – Fully/partially interconnected nodes  
- 🔄 **Ring** – Circular arrangement with each node connected to two neighbors  
- ⭐ **Star** – Central hub node with all others connected directly to it  

These topologies offer varying levels of connectivity and congestion sensitivity.

---

## 🎯 3. Objective: Minimization of the Congestion Metric

Our primary objective is to **minimize the Congestion Metric (CM)** using an **adaptive congestion window (cwnd) update strategy**, governed by:

### 🔧 Control Equation
Δcwnd = (1 - PL) · (a / (b + cwnd)) - PL · (c · cwnd - d)

Where:
- **Δcwnd** – Change in the congestion window
- **PL** – Instantaneous packet-loss probability
- **a**, **b** – Growth parameters
  - `a` controls aggressiveness under low-loss conditions
  - `b` prevents rapid increase when `cwnd` is small
- **c**, **d** – Reduction parameters
  - `c` handles multiplicative decrease upon loss
  - `d` adds a constant decrement to reduce "train" behavior
- **cwnd** – Current congestion window size (in bytes or segments)

### 📦 Case Configurations

#### ➤ TCP Tahoe:
a = 1, b = 0, c = 1, d = 1 Δcwnd = (1 / cwnd) - PL · (1 / cwnd + (cwnd - 1))

#### ➤ TCP Reno:
a = 1, b = 0, c = 0.5, d = 0 Δcwnd = (1 / cwnd) - PL · (cwnd / 2 + 1 / cwnd)

## 📈 4. Gradient‑Descent‑Driven Parameter Tuning

To optimize parameters `{a, b, c, d}` for minimizing congestion, we implement **online optimization** using **gradient descent**.

### 🔄 Update Procedure

After each measurement interval:
1. **Compute CM**:
CM = α · D - β · T + γ · J


2. **Estimate Gradients**:
Using finite differences:
∂CM/∂x ≈ (CM(x + ε) - CM(x)) / ε


3. **Parameter Update**:
Move in the direction of the negative gradient:
x ← x - η · ∂CM/∂x for x ∈ {a, b, c, d}

where `η` is the learning rate.

### ✅ Convergence Criteria

Iterate until:
- The **relative change in CM** falls below a preset threshold.
- The parameter set `{a, b, c, d}` stabilizes at a near-optimal point.

---

## ⚙️ Summary

This approach unifies:
- 🔄 **Real-time congestion control** (via Δcwnd)
- 🧠 **Offline optimization** (via gradient descent)

...to dynamically **drive the network toward a minimal congestion state** by learning the best control parameters based on observed network feedback.

---



