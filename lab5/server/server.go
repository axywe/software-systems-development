package main

import (
	"context"
	"errors"
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
	if len(matrix) != size*size || len(constants) != size {
		return nil, errors.New("invalid input dimensions")
	}

	for i := 0; i < size; i++ {
		maxEl := math.Abs(matrix[i*size+i])
		maxRow := i
		for k := i + 1; k < size; k++ {
			if math.Abs(matrix[k*size+i]) > maxEl {
				maxEl = math.Abs(matrix[k*size+i])
				maxRow = k
			}
		}

		if maxEl == 0 {
			return nil, errors.New("matrix is singular")
		}

		if maxRow != i {
			for k := i; k < size; k++ {
				matrix[maxRow*size+k], matrix[i*size+k] = matrix[i*size+k], matrix[maxRow*size+k]
			}
			constants[maxRow], constants[i] = constants[i], constants[maxRow]
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

	solution := make([]float64, size)
	for i := size - 1; i >= 0; i-- {
		solution[i] = math.Round(constants[i]/matrix[i*size+i]*1e12) / 1e12
		for k := i - 1; k >= 0; k-- {
			constants[k] -= matrix[k*size+i] * solution[i]
		}
	}

	for i := 0; i < size; i++ {
		if math.Abs(matrix[i*size+i]) < 1e-10 {
			return nil, errors.New("matrix is singular")
		}
	}
	log.Printf("Solution: %v\n", solution)
	return solution, nil
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
