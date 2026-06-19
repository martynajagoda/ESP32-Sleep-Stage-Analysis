This project implements a low-cost, edge-computing sleep monitoring system using an ESP32 microcontroller and a comprehensive Machine Learning pipeline. By fusing Photoplethysmography (PPG) and Actigraphy data, the system classifies human sleep into four distinct stages: Wake, Light Sleep, Deep Sleep, and REM (Rapid Eye Movement).
The project demonstrates the complete Data Science workflow: from hardware assembly and signal processing (C++) to feature engineering, hyperparameter optimization, and medical-grade data visualization (Python).

Hardware Architecture 
The physical data collection unit is built using:
ESP32: Dual-core microcontroller with built-in Wi-Fi, hosting a local live dashboard.
MAX30102: Optical pulse oximeter for tracking Heart Rate (BPM) and Blood Oxygen Saturation (SpO2).
LSM9DS1: 9-DOF IMU sensor used as an actigraph to detect micro-movements.

Feature Engineering at the Edge 
Instead of streaming raw, noisy data, the ESP32 performs on-device feature extraction based on clinical 30-second epochs:
Heart Rate Variability (HRV): Calculated as the standard deviation of the heart rate over the epoch. High HRV correlates with Deep Sleep (parasympathetic nervous system dominance), while low HRV indicates REM or Wakefulness.
3D Movement Magnitude: The 3-axis accelerometer data (X, Y, Z) is compressed into a single vector using the Pythagorean theorem, with Earth's gravity ($9.81 m/s^2$) subtracted. This ensures accurate movement detection regardless of the sensor's orientation during sleep.
Average BPM & SpO2: Aggregated physiological baselines.

Machine Learning Pipeline 
The collected CSV datasets are processed using a sophisticated Python pipeline:
Model: Random Forest Classifier (scikit-learn).
Hyperparameter Tuning: Automated optimization using Optuna to find the best tree depth, estimators, and split criteria.
Validation: Stratified K-Fold Cross-Validation to ensure the model generalizes well to unseen data.
Evaluation: Comprehensive metrics including Confusion Matrices, Feature Importance bar charts, and Classification Reports.

Data Visualization & Clinical Outputs 
The pipeline automatically generates clinical-style visualizations:
Comparative Hypnograms: Step-plots comparing the sleep architecture of a healthy night versus a stressed/disturbed night.
SpO2 Time-Series: Blood oxygen levels plotted against standard medical thresholds (Normal, Mild Hypoxia, Hypoxia).
KDE Density Plots: Evaluating the distribution of SpO2 levels throughout the night.

Running the Project
Flash the ESP32_Sleep_Tracker.ino to the board and connect to its local IP to monitor live signals.
Record night data via serial capture (e.g., CoolTerm) to a .csv file.
Open Sleep_ML_Pipeline.ipynb in Google Colab, upload the CSV, and run all cells to generate the hypnogram.
