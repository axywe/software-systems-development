# Equation Solver Service

This project provides a simple web-based interface to solve systems of linear equations using gRPC and Go. It consists of two main components: a gRPC server that computes the solution of the equations, and a web server that serves as the user interface for entering equations and displaying solutions.

## Project Structure

- `client/`: Contains the Go source file for the HTTP server which interacts with users and communicates with the gRPC server.
- `server/`: Contains the Go source file for the gRPC server that handles the logic for solving equations.
- `computation/`:
  - `solver.proto`: Protocol buffers file for defining the service and messages.
  - `solver.pb.go`: Generated code from the protocol buffers file.
  - `solver_grpc.pb.go`: Generated gRPC code from the protocol buffers file.
- `README.md`: This file, which provides documentation for the project.
- `go.mod` and `go.sum`: Go module files for managing dependencies.

## Requirements

- Go (version 1.15 or later recommended)
- Protocol Buffers Compiler (protoc)
- gRPC

## Setup Instructions

### Running the Servers

1. **Start the gRPC Server:**
   Navigate to the `server/` directory and run:

   ```bash
   go run server.go
   ```

   This starts the gRPC server on port `50051`.

2. **Start the Web Server:**
   Navigate to the `client/` directory and run:

   ```bash
   go run client.go
   ```

   This starts the web server on port `8080`.

## Using the Application

1. Open your web browser and navigate to `http://localhost:8080`.
2. Enter the size of the matrix and the coefficients for your system of equations.
3. Click "Solve" to see the solution or error message.
