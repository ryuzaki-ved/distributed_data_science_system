#!/bin/bash

# Distributed Data Science System Build Script
# This script builds the entire DDS system with all components

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="Distributed Data Science System"
PROJECT_VERSION="1.0.0"
BUILD_DIR="build"
INSTALL_DIR="install"
BUILD_TYPE="Release"
NUM_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for CMake
    if ! command_exists cmake; then
        missing_deps+=("cmake")
    fi
    
    # Check for C++ compiler
    if ! command_exists g++ && ! command_exists clang++; then
        missing_deps+=("C++ compiler (g++ or clang++)")
    fi
    
    # Check for MPI
    if ! command_exists mpicc; then
        missing_deps+=("MPI (mpicc)")
    fi
    
    # Check for make
    if ! command_exists make; then
        missing_deps+=("make")
    fi
    
    # Check for git
    if ! command_exists git; then
        missing_deps+=("git")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "Please install the missing dependencies and try again."
        exit 1
    fi
    
    print_success "All dependencies found"
}

# Function to detect system
detect_system() {
    print_status "Detecting system configuration..."
    
    # Detect OS
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="Linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macOS"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        OS="Windows"
    else
        OS="Unknown"
    fi
    
    # Detect architecture
    ARCH=$(uname -m)
    
    # Detect compiler
    if command_exists g++; then
        COMPILER="gcc"
        COMPILER_VERSION=$(g++ --version | head -n1 | awk '{print $NF}')
    elif command_exists clang++; then
        COMPILER="clang"
        COMPILER_VERSION=$(clang++ --version | head -n1 | awk '{print $NF}')
    else
        COMPILER="unknown"
        COMPILER_VERSION="unknown"
    fi
    
    # Detect MPI
    if command_exists mpicc; then
        MPI_VERSION=$(mpicc --version | head -n1 | awk '{print $NF}')
    else
        MPI_VERSION="unknown"
    fi
    
    print_success "System: $OS ($ARCH), Compiler: $COMPILER $COMPILER_VERSION, MPI: $MPI_VERSION"
}

# Function to create build directory
create_build_dir() {
    print_status "Creating build directory..."
    
    if [ -d "$BUILD_DIR" ]; then
        print_warning "Build directory already exists, cleaning..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    print_success "Build directory created: $BUILD_DIR"
}

# Function to configure CMake
configure_cmake() {
    print_status "Configuring CMake..."
    
    cd "$BUILD_DIR"
    
    # CMake configuration options
    local cmake_options=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_INSTALL_PREFIX=../$INSTALL_DIR"
        "-DCMAKE_CXX_STANDARD=17"
        "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        "-DBUILD_SHARED_LIBS=OFF"
        "-DBUILD_TESTS=ON"
        "-DBUILD_EXAMPLES=ON"
        "-DBUILD_DOCUMENTATION=OFF"
        "-DENABLE_COVERAGE=OFF"
        "-DENABLE_PROFILING=OFF"
        "-DENABLE_DEBUG=OFF"
    )
    
    # Add system-specific options
    if [[ "$OS" == "macOS" ]]; then
        cmake_options+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15")
    fi
    
    # Run CMake
    if cmake .. "${cmake_options[@]}"; then
        print_success "CMake configuration completed"
    else
        print_error "CMake configuration failed"
        exit 1
    fi
    
    cd ..
}

# Function to build the project
build_project() {
    print_status "Building project with $NUM_JOBS jobs..."
    
    cd "$BUILD_DIR"
    
    if make -j"$NUM_JOBS"; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
    
    cd ..
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    cd "$BUILD_DIR"
    
    if make test; then
        print_success "All tests passed"
    else
        print_warning "Some tests failed"
    fi
    
    cd ..
}

# Function to install
install_project() {
    print_status "Installing project..."
    
    cd "$BUILD_DIR"
    
    if make install; then
        print_success "Installation completed"
    else
        print_error "Installation failed"
        exit 1
    fi
    
    cd ..
}

# Function to create package
create_package() {
    print_status "Creating package..."
    
    cd "$BUILD_DIR"
    
    if make package; then
        print_success "Package created successfully"
    else
        print_warning "Package creation failed"
    fi
    
    cd ..
}

# Function to clean build
clean_build() {
    print_status "Cleaning build directory..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    fi
}

# Function to show help
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -d, --debug         Build in debug mode"
    echo "  -j, --jobs N        Number of parallel jobs (default: $NUM_JOBS)"
    echo "  -t, --test          Run tests after building"
    echo "  -i, --install       Install after building"
    echo "  -p, --package       Create package after building"
    echo "  --clean-only        Only clean build directory"
    echo "  --configure-only    Only configure CMake"
    echo "  --build-only        Only build (skip configure)"
    echo ""
    echo "Examples:"
    echo "  $0                  # Full build with default settings"
    echo "  $0 -c -d            # Clean build in debug mode"
    echo "  $0 -j 8 -t -i       # Build with 8 jobs, run tests, and install"
    echo "  $0 --clean-only     # Only clean build directory"
}

# Main function
main() {
    echo "=========================================="
    echo "  $PROJECT_NAME v$PROJECT_VERSION"
    echo "=========================================="
    echo ""
    
    # Parse command line arguments
    local clean_build_flag=false
    local debug_mode=false
    local run_tests_flag=false
    local install_flag=false
    local package_flag=false
    local clean_only=false
    local configure_only=false
    local build_only=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                clean_build_flag=true
                shift
                ;;
            -d|--debug)
                debug_mode=true
                BUILD_TYPE="Debug"
                shift
                ;;
            -j|--jobs)
                NUM_JOBS="$2"
                shift 2
                ;;
            -t|--test)
                run_tests_flag=true
                shift
                ;;
            -i|--install)
                install_flag=true
                shift
                ;;
            -p|--package)
                package_flag=true
                shift
                ;;
            --clean-only)
                clean_only=true
                shift
                ;;
            --configure-only)
                configure_only=true
                shift
                ;;
            --build-only)
                build_only=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Handle clean-only mode
    if [ "$clean_only" = true ]; then
        clean_build
        exit 0
    fi
    
    # Check dependencies
    check_dependencies
    
    # Detect system
    detect_system
    
    # Clean build if requested
    if [ "$clean_build_flag" = true ]; then
        clean_build
    fi
    
    # Create build directory
    create_build_dir
    
    # Configure CMake
    if [ "$build_only" = false ]; then
        configure_cmake
    fi
    
    # Handle configure-only mode
    if [ "$configure_only" = true ]; then
        print_success "Configuration completed"
        exit 0
    fi
    
    # Build project
    build_project
    
    # Handle build-only mode
    if [ "$build_only" = true ]; then
        print_success "Build completed"
        exit 0
    fi
    
    # Run tests if requested
    if [ "$run_tests_flag" = true ]; then
        run_tests
    fi
    
    # Install if requested
    if [ "$install_flag" = true ]; then
        install_project
    fi
    
    # Create package if requested
    if [ "$package_flag" = true ]; then
        create_package
    fi
    
    echo ""
    echo "=========================================="
    print_success "Build process completed successfully!"
    echo "=========================================="
    echo ""
    echo "Build directory: $BUILD_DIR"
    echo "Install directory: $INSTALL_DIR"
    echo "Build type: $BUILD_TYPE"
    echo ""
    
    if [ "$install_flag" = true ]; then
        echo "To run the system:"
        echo "  cd $INSTALL_DIR"
        echo "  ./bin/dds_client --help"
        echo ""
    fi
}

# Run main function with all arguments
main "$@" 