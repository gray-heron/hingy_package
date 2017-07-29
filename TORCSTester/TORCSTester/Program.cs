using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TORCSTester
{
    class Program
    {
        public static void StandardFeedback(string line) {
            Console.WriteLine(line);
        }    

        static void Main(string[] args)
        {
            if(args.Length != 2)
            {
                Console.WriteLine("Usage: runner.exe config.xml threads_count");
            }

	    System.Diagnostics.Process.Start("killall", "-9 hingybot torcs-bin").WaitForExit();

            var test = Configuration.Read(args[0]);
            var tester = new Runner(test);
            var results = tester.Run(test.cases, System.Convert.ToInt32(args[1]), StandardFeedback);
            var mean_p = 0.0;
            float total_score = 0.0f;

	    Console.WriteLine(test.cases.Count);

            Console.WriteLine("--------------------------");
            foreach (var result in results)
            {
                var r = result.Item2 - result.Item1.ref1;
                var p = r / result.Item1.ref1 * 100.0f;
                mean_p += p;
                Console.WriteLine("Case " + result.Item1.name + ": " + result.Item2.ToString() + " ref " +
                    r.ToString() + " (" + p.ToString() + "%)");
                total_score += result.Item2;
            }

            mean_p /= (float)results.Count;
            Console.WriteLine("Total score: " + total_score);
            Console.WriteLine("Mean score: " + total_score / (float)results.Count);
            Console.WriteLine("Mean ref: " + mean_p);
            Console.WriteLine("--------------------------");
        }
    }
}
