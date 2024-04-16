# Distributed Data Science Framework

A high-performance distributed data science framework built with C++, MPI, and Hadoop for scalable machine learning and data processing.

## Overview

This framework enables distributed execution of data science tasks including:
- Linear Regression
- Logistic Regression  
- K-means Clustering
- DBSCAN Clustering

The system leverages C++ for performance, MPI for inter-node communication, and Hadoop for distributed storage and fault tolerance.

## Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Client Layer  │    │  Job Manager    │    │  Worker Nodes   │
│   (CLI/Web UI)  │◄──►│   (Master)      │◄──►│   (MPI/Compute) │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │
                                ▼
                       ┌─────────────────┐
                       │   Hadoop HDFS   │
                       │  (Distributed   │
                       │   Storage)      │
                       └─────────────────┘
```

## Key Components

### 1. Client Layer
- Command-line interface for job submission
- Web-based monitoring dashboard
- Job status tracking and result retrieval

### 2. Job Manager
- Job scheduling and distribution
- Worker node management
- Fault tolerance and recovery
- Resource allocation

### 3. Worker Nodes
- MPI-based parallel computation
- Algorithm implementations (regression, clustering)
- Data partitioning and processing
- Result aggregation

### 4. Hadoop Integration
- HDFS for distributed data storage
- YARN for resource management
- MapReduce for data preprocessing

## Prerequisites

- C++17 or later
- MPI (OpenMPI or MPICH)
- Hadoop 3.x
- Boost Libraries
- Eigen3 (for linear algebra)
- CMake 3.15+

## Installation

### 1. Install Dependencies

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libboost-all-dev libeigen3-dev openmpi-bin libopenmpi-dev
```

#### CentOS/RHEL:
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake boost-devel eigen3-devel openmpi-devel
```

#### macOS:
```bash
brew install cmake boost eigen open-mpi
```

### 2. Install Hadoop
```bash
# Download and extract Hadoop
wget https://downloads.apache.org/hadoop/common/hadoop-3.3.6/hadoop-3.3.6.tar.gz
tar -xzf hadoop-3.3.6.tar.gz
sudo mv hadoop-3.3.6 /opt/hadoop

# Set environment variables
export HADOOP_HOME=/opt/hadoop
export PATH=$PATH:$HADOOP_HOME/bin:$HADOOP_HOME/sbin
```

### 3. Build the Framework
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Usage

### 1. Start Hadoop Cluster
```bash
# Start HDFS
start-dfs.sh

# Start YARN
start-yarn.sh
```

### 2. Submit a Job
```bash
# Linear Regression
./bin/dds_client --job-type=linear_regression --data-path=/data/regression.csv --output-path=/results/

# K-means Clustering
./bin/dds_client --job-type=kmeans --data-path=/data/clustering.csv --k=5 --output-path=/results/

# Logistic Regression
./bin/dds_client --job-type=logistic_regression --data-path=/data/classification.csv --output-path=/results/
```

### 3. Monitor Jobs
```bash
# Check job status
./bin/dds_client --status --job-id=<job_id>

# List all jobs
./bin/dds_client --list-jobs
```

## Configuration

### MPI Configuration
Edit `config/mpi_config.json`:
```json
{
  "hostfile": "config/hosts.txt",
  "processes_per_node": 4,
  "timeout": 300
}
```

### Hadoop Configuration
Edit `config/hadoop_config.json`:
```json
{
  "hdfs_uri": "hdfs://localhost:9000",
  "yarn_uri": "http://localhost:8088",
  "replication_factor": 3
}
```

## Project Structure

```
Distributed Data Science System/
├── src/
│   ├── client/           # Client application
│   ├── job_manager/      # Job scheduling and management
│   ├── worker/           # Worker node implementation
│   ├── algorithms/       # ML algorithm implementations
│   ├── communication/    # MPI communication layer
│   ├── storage/          # HDFS integration
│   └── utils/            # Utility functions
├── include/              # Header files
├── tests/                # Unit and integration tests
├── config/               # Configuration files
├── scripts/              # Build and deployment scripts
├── docs/                 # Documentation
└── examples/             # Example usage
```

## Algorithms

### Linear Regression
- Parallel gradient computation
- Distributed parameter updates
- Row-based data partitioning

### Logistic Regression
- Binary and multi-class classification
- Parallel gradient descent
- Regularization support

### K-means Clustering
- Parallel centroid computation
- Distributed point assignment
- Automatic convergence detection

### DBSCAN Clustering
- Parallel density calculation
- Distributed cluster expansion
- Efficient neighbor search

## Performance

The framework is designed for high-performance distributed computing:

- **Scalability**: Linear scaling with number of nodes
- **Fault Tolerance**: Automatic recovery from node failures
- **Memory Efficiency**: Streaming data processing for large datasets
- **Communication Optimization**: Minimal MPI communication overhead

## Monitoring

- Real-time job progress tracking
- Resource utilization monitoring
- Performance metrics collection
- Error logging and debugging

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

MIT License - see LICENSE file for details.

## Support

For issues and questions:
- Create an issue on GitHub
- Check the documentation in `docs/`
- Review example usage in `examples/` 