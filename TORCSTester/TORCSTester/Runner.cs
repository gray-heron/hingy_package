using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.IO;


namespace TORCSTester
{
    public class Runner
    {
        Configuration config;
        ConcurrentQueue<TestCase> pending_cases = new ConcurrentQueue<TestCase>();
        ConcurrentQueue<Tuple<TestCase, float>> results;
        Action<string> feedback;

        void RunCase(TestCase test) {
            float score = 0.0f;
            bool released = false;

            var server_proc = new Process();
            server_proc.StartInfo.Arguments = config.server_args + " " + test.server_args;
            server_proc.StartInfo.FileName = config.server_executable;
            server_proc.StartInfo.WorkingDirectory = config.server_path;
            server_proc.StartInfo.RedirectStandardOutput = true;
            server_proc.StartInfo.UseShellExecute = false;
	    
            var client_proc = new Process();
            client_proc.StartInfo.Arguments = config.client_args + " " + test.client_args;
            client_proc.StartInfo.FileName = config.client_executable;
            client_proc.StartInfo.WorkingDirectory = config.client_path;
            client_proc.StartInfo.UseShellExecute = false;
            client_proc.StartInfo.CreateNoWindow = true;
            client_proc.StartInfo.RedirectStandardOutput = true;
	    client_proc.StartInfo.UseShellExecute = false;

            server_proc.Start();
            client_proc.Start();
            
            while (!server_proc.StandardOutput.EndOfStream)
            {
                string line = server_proc.StandardOutput.ReadLine();

                if (!released && config.server_confirmation_msg == line)
                {
                    released = true;
                }

                float? round_score = ExtractScore(config.server_score_captureline, line);
                if (round_score != null)
                {
                    score = round_score.Value;
		    
		    try {
		    	server_proc.Kill();
		    }
		    catch ( InvalidOperationException ) {}
                    try {
		    	client_proc.Kill();
		    }
		    catch ( InvalidOperationException ) {}     

		    break;
                }
            }

	    server_proc.WaitForExit();
	    client_proc.WaitForExit();
	    
	    results.Enqueue(new Tuple<TestCase, float>(test, score));
        }

        void Worker() {
            TestCase test;
            while(pending_cases.TryDequeue(out test)) {
                feedback("Case " + test.name + " has started.");
                RunCase(test);
                feedback("Case " + test.name + " has ended.");
            }
        }

        public List<Tuple<TestCase, float>> Run(List<TestCase> cases, 
            int threads_n, Action<string> feedback)
        {
            if (!pending_cases.IsEmpty)
                throw new Exception();

            List<Thread> threads = new List<Thread>();
                
            pending_cases = new ConcurrentQueue<TestCase>(cases);
            results = new ConcurrentQueue<Tuple<TestCase, float>>();
            this.feedback = feedback;

            for(int i = 0; i < threads_n; i++)
            {
                Thread worker = new Thread(Worker);
                threads.Add(worker);
                worker.Start();
            }

            foreach(var worker in threads)
            {
                worker.Join();
            }

            var end_results = new List<Tuple<TestCase, float>>();

            foreach (var result in results)
            {
                if (result.Item2 == 0.0f)
                    end_results.Add(new Tuple<TestCase, float>(result.Item1, result.Item1.ref1 * 21.0f));
                else
                    end_results.Add(result);
            }

            return end_results;
        }

        public Runner(Configuration config) {
            this.config = config;
        }

        float? ExtractScore(string pattern, string input) {
            input = input.Replace(" ", "");
            pattern = pattern.Replace(" ", "");

            int i = 0;
            string score = "";

            while(i != input.Length && pattern[i] != '#') {
                if (pattern[i] != input[i])
                    return null;

                i += 1;
            }

            while ("1234567890".Contains(input[i]))
                score += input[i++];

            if (input[i++] == '.')
            {
                score += '.';
                while (i != input.Length && "1234567890".Contains(input[i]))
                    score += input[i++];
            }
            
            while (i - score.Length + 1 != pattern.Length)
            {
                if (pattern[i - score.Length + 1] != input[i])
                    return null;

                i += 1;
            }
            return float.Parse(score, System.Globalization.NumberStyles.Any);
        }
    }
}
