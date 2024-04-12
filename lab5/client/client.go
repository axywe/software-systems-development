package main

import (
	"context"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"time"

	pb "solver/computation"

	"google.golang.org/grpc"
)

type Server struct {
	solverClient pb.SolverClient
}

func (s *Server) serveForm(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusOK)
	fmt.Fprintf(w, `
		<!DOCTYPE html>
		<html>
		<head>
			<title>Equation Solver</title>
			<style>
			body {
				font-family: Arial, sans-serif;
				background-color: #f4f4f9;
				color: #333;
				padding: 20px;
			}
			
			form {
				background-color: #ffffff;
				padding: 20px;
				border-radius: 8px;
				box-shadow: 0 4px 8px rgba(0,0,0,0.1);
				width: fit-content;
				margin: auto;
			}
			
			#matrix, #constants {
				display: inline-block;
				vertical-align: top;
				margin-right: 20px;
			}
			
			input[type="text"] {
				width: 40px;
				margin: 2px;
				padding: 5px;
				border: 1px solid #ccc;
				border-radius: 4px;
			}
			
			input[type="number"] {
				padding: 5px;
				border-radius: 4px;
				border: 1px solid #ccc;
			}
			
			input[type="submit"] {
				background-color: #4CAF50;
				color: white;
				border: none;
				padding: 10px 20px;
				text-align: center;
				text-decoration: none;
				display: block;
				font-size: 16px;
				margin: 10px auto;
				cursor: pointer;
				border-radius: 4px;
			}
			
			label {
				font-weight: bold;
			}
			
		</style>
			<script>
				function updateFields() {
					var size = document.getElementById('size').value;
					var matrix = document.getElementById('matrix');
					var constants = document.getElementById('constants');
					var newSize = parseInt(size);
					if (!isNaN(newSize)) {
						var matrixHTML = '';
						var constantsHTML = '';
						for (var i = 0; i < newSize; i++) {
							for (var j = 0; j < newSize; j++) {
								matrixHTML += '<input type="text" required name="m' + i + '_' + j + '" style="width: 40px;"/>';
							}
							matrixHTML += '<br/>';
							constantsHTML += '<input type="text" required name="c' + i + '" style="width: 40px;"/><br/>';
						}
						matrix.innerHTML = matrixHTML;
						constants.innerHTML = constantsHTML;
					}
				}
			</script>
		</head>
		<body>
			<form action="/solve" method="post">
				<label for="size">Matrix Size:</label>
				<input type="number" id="size" name="size" oninput="updateFields()" min="1"/><br/>
				<div id="matrix"></div>
				<div id="constants"></div><br>
				<input type="submit" value="Solve"/>
			</form>
		</body>
		</html>
	`)
}

func (s *Server) serveSolution(w http.ResponseWriter, msg string, status int) {
	w.WriteHeader(status)
	fmt.Fprintf(w, `
		<!DOCTYPE html>
	<html>
	<head>
		<title>Solution</title>
		<style>
			body {
				font-family: Arial, sans-serif;
				background-color: #f4f4f9;
				color: #333;
				padding: 20px;
				text-align: center;
			}
			
			#solution {
				background-color: #ffffff;
				padding: 20px;
				border-radius: 8px;
				box-shadow: 0 4px 8px rgba(0,0,0,0.1);
				display: inline-block;
				margin-top: 20px;
			}
			
			a {
				display: inline-block;
				background-color: #4CAF50;
				color: white;
				padding: 10px 20px;
				text-decoration: none;
				font-size: 16px;
				border-radius: 4px;
				margin-top: 20px;
			}
		</style>
	</head>
	<body>
		<div id="solution">
			<h2>%v</h2>
			<a onclick="window.history.back();">Back</a>
		</div>
	</body>
	</html>
	`, msg)
}

func (s *Server) solveEquation(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Redirect(w, r, "/", http.StatusSeeOther)
		return
	}

	err := r.ParseForm()
	if err != nil {
		s.serveSolution(w, "Error: Error parsing form", http.StatusBadRequest)
		return
	}

	sizeStr := r.FormValue("size")
	size, err := strconv.Atoi(sizeStr)
	if err != nil || size < 1 {
		s.serveSolution(w, "Error: Invalid matrix size", http.StatusBadRequest)
		return
	}

	matrix := make([]float64, size*size)
	constants := make([]float64, size)

	for i := 0; i < size; i++ {
		for j := 0; j < size; j++ {
			mVal, err := strconv.ParseFloat(r.FormValue(fmt.Sprintf("m%d_%d", i, j)), 64)
			if err != nil {
				s.serveSolution(w, "Error: Invalid matrix values", http.StatusBadRequest)
				return
			}
			matrix[i*size+j] = mVal
		}
		cVal, err := strconv.ParseFloat(r.FormValue(fmt.Sprintf("c%d", i)), 64)
		if err != nil {
			s.serveSolution(w, "Error: Invalid constants values", http.StatusBadRequest)
			return
		}
		constants[i] = cVal
	}

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	res, err := s.solverClient.SolveEquation(ctx, &pb.EquationRequest{
		Size:      int32(size),
		Matrix:    matrix,
		Constants: constants,
	})
	if err != nil {
		s.serveSolution(w, "Error connecting to server", http.StatusInternalServerError)
		return
	}

	if res.Error != "" {
		s.serveSolution(w, fmt.Sprintf("Solver error: %v\n", res.Error), http.StatusBadRequest)
		return
	}

	resultString := fmt.Sprintf("Solution: %v", res.GetSolution())
	res.GetSolution()
	s.serveSolution(w, resultString, http.StatusOK)
}

func main() {
	conn, err := grpc.Dial("localhost:50051", grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()

	client := pb.NewSolverClient(conn)
	server := &Server{solverClient: client}

	http.HandleFunc("/", server.serveForm)
	http.HandleFunc("/solve", server.solveEquation)
	fmt.Printf("Starting server on port 8080\n")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
