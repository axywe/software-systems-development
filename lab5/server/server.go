package main

import (
	"context"
	"fmt"
	"log"
	"math"
	"net"

	pb "solver/computation" 

	"google.golang.org/grpc"
)

type server struct {
	pb.UnimplementedSolverServer
}

func solveLinearSystem(size int, matrix, constants []float64) ([]float64, error) {
	for i := 0; i < size; i++ {
		maxRow := i
		for k := i + 1; k < size; k++ {
			if abs(matrix[k*size+i]) > abs(matrix[maxRow*size+i]) {
				maxRow = k
			}
		}

		if i != maxRow {
			for k := i; k < size; k++ {
				matrix[i*size+k], matrix[maxRow*size+k] = matrix[maxRow*size+k], matrix[i*size+k]
			}
			constants[i], constants[maxRow] = constants[maxRow], constants[i]
		}

		for k := i + 1; k < size; k++ {
			c := -matrix[k*size+i] / matrix[i*size+i]
			for j := i; j < size; j++ {
				if i == j {
					matrix[k*size+j] = 0
				} else {
					matrix[k*size+j] += c * matrix[i*size+j]
				}
			}
			constants[k] += c * constants[i]
		}
	}

	for i := size - 1; i >= 0; i-- {
		if matrix[i*size+i] == 0 && constants[i] != 0 {
			return nil, fmt.Errorf("no solution")
		}
	}

	solution := make([]float64, size)
	for i := size - 1; i >= 0; i-- {
		sum := constants[i]
		for j := i + 1; j < size; j++ {
			sum -= matrix[i*size+j] * solution[j]
		}
		solution[i] = math.Round(sum/matrix[i*size+i]*1e12) / 1e12
	}

	log.Printf("Solution: %v\n", solution)

	return solution, nil
}

func abs(x float64) float64 {
	if x < 0 {
		return -x
	}
	return x
}

func (s *server) SolveEquation(ctx context.Context, req *pb.EquationRequest) (*pb.EquationResponse, error) {
	solution, err := solveLinearSystem(int(req.Size), req.Matrix, req.Constants)
	if err != nil {
		return &pb.EquationResponse{Error: err.Error()}, nil
	}
	return &pb.EquationResponse{Solution: solution}, nil
}

func main() {
	lis, err := net.Listen("tcp", ":50051")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}
	s := grpc.NewServer()
	pb.RegisterSolverServer(s, &server{})
	fmt.Printf("Starting server\n")
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
