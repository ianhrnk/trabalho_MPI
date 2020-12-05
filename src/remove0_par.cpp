/********************************************************************************
 * Trabalho 3: Programação Paralela para Multiprocessador com Memória Distribuída
 * Professora: Nahri Moreano
 * Aluno: Ian Haranaka | RGA: 2018.1904.009-7
 * Comando de compilação: mpic++ remove0_par.cpp -o remove0_par -Wall
 ********************************************************************************/

#include <iostream>
#include <fstream>
#include <mpi.h>

#define RAIZ 0

int* AlocaVetor(int n);
void RemoveZeros(int n, int *v_in, int *m, int *v_out);
void CalculaDeslocamento(int *displs, int *recvcounts, int num_proc);

int main(int argc, char *argv[])
{
  int n, m, *v_in, *v_out, id_proc, num_proc, n_itens_proc, m_restante = 0;
  int *recvcounts, *displs;
  double t_ini, t_fim, tempo_local, tempo_max;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &id_proc);
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

  // Recebendo entrada
  if (id_proc == 0)
  {
    std::ifstream entrada(argv[1]);

    if (entrada.is_open())
    {
      entrada >> n;

      v_in = AlocaVetor(n);
      v_out = AlocaVetor(n);
      for (int i = 0; i < n; ++i)
        entrada >> v_in[i];

      entrada.close();
    }
    else
    {
      std::cout << "Arquivo não encontrado!" << std::endl;
      return -1;
    }

    n_itens_proc = n / num_proc;
  }

  // Início da medição de tempo
  MPI_Barrier(MPI_COMM_WORLD);
  t_ini = MPI_Wtime();

  // Processo raiz envia o número de elementos que cada processo vai receber
  MPI_Bcast(&n_itens_proc, 1, MPI_INT, RAIZ, MPI_COMM_WORLD);

  if (id_proc == 0)
  {
    recvcounts = AlocaVetor(num_proc);
    displs = AlocaVetor(num_proc);
  }
  else
  {
    v_in = AlocaVetor(n_itens_proc);
    v_out = AlocaVetor(n_itens_proc);
  }

  // Processo raiz distribui os elementos do vetor
  MPI_Scatter(v_in, n_itens_proc, MPI_INT, v_in, n_itens_proc, MPI_INT, RAIZ, MPI_COMM_WORLD);
  RemoveZeros(n_itens_proc, v_in, &m, v_out);

  // Processo raiz recebe o número de elementos restantes de cada processo
  MPI_Gather(&m, 1, MPI_INT, recvcounts, 1, MPI_INT, RAIZ, MPI_COMM_WORLD);
  if (id_proc == 0) CalculaDeslocamento(displs, recvcounts, num_proc);

  // Processo raiz recebe os elementos restantes dos outros processos
  MPI_Gatherv(v_out, m, MPI_INT, v_out, recvcounts, displs, MPI_INT, RAIZ, MPI_COMM_WORLD);

  // P/ descobrir o número total de elementos restantes
  MPI_Reduce(&m, &m_restante, 1, MPI_INT, MPI_SUM, RAIZ, MPI_COMM_WORLD);

  // Fim da medição de tempo
  t_fim = MPI_Wtime();
  tempo_local = t_fim - t_ini;
  MPI_Reduce(&tempo_local, &tempo_max, 1, MPI_DOUBLE, MPI_MAX, RAIZ, MPI_COMM_WORLD);

  // Escrevendo saída
  if (id_proc == 0)
  {
    std::cout << "Tempo de execução = " << tempo_max << " segundos" << std::endl;
    std::ofstream saida(argv[2]);

    if (saida.is_open())
    {
      saida << m_restante << '\n';

      for (int i = 0; i < n && v_out[i] != 0; ++i)
        saida << v_out[i] << ' ';

      saida << '\n';
      saida.close();
    }
  }

  if (id_proc == 0)
  {
    delete recvcounts;
    delete displs;
  }
  delete v_in;
  delete v_out;

  MPI_Finalize();
  return 0;
}

int* AlocaVetor(int n)
{
  int *vetor = new (std::nothrow) int[n]();
  if (vetor == nullptr)
    std::cout << "Erro na alocação de memória!" << std::endl;
  return vetor;
}

void RemoveZeros(int n, int *v_in, int *m, int *v_out)
{
  int c = 0;
  for (int i = 0; i < n; i++)
  {
    if (v_in[i] != 0)
    {
      v_out[c] = v_in[i];
      c++;
    }
  }
  *m = c;
}

void CalculaDeslocamento(int *displs, int *recvcounts, int num_proc)
{
  for (int i = 1; i < num_proc; ++i)
    displs[i] = displs[i-1] + recvcounts[i-1];
}
