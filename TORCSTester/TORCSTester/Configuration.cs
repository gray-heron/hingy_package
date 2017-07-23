using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace TORCSTester
{
    public class TestCase {
        public string name, client_args, server_args;
        public string config_file;

        public float ref1;
    }

    public class Configuration
    {
        public string server_executable, server_args, server_path;
        public string client_executable, client_args, client_path;
        public string server_config_file, server_score_captureline;
        public string server_confirmation_msg;
        public List<TestCase> cases = new List<TestCase>();

        public void Write(string file)
        {
            using (var writer = new System.IO.StreamWriter(file))
            {
                var serializer = new XmlSerializer(typeof(Configuration));
                serializer.Serialize(writer, this);
                writer.Flush();
            }
        }

        public static Configuration Read(string file)
        {
            using (var reader = new System.IO.StreamReader(file))
            {
                var serializer = new XmlSerializer(typeof(Configuration));
                var obj = serializer.Deserialize(reader);

                return (Configuration)obj;
            }
        }
    }
}
