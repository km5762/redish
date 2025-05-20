import time
import unittest

from redis import ResponseError
from redis.connection import Connection


class Requests(unittest.TestCase):
    # @classmethod
    # def setUpClass(cls):
    #     project_root = os.path.abspath(os.path.dirname(__file__))
    #     build_dir = os.path.abspath(os.path.join(project_root, '../cmake-build-debug'))
    #     os.makedirs(build_dir, exist_ok=True)
    #     subprocess.run(['cmake', '..'], cwd=build_dir, check=True)
    #     subprocess.run(['cmake', '--build', '.', '--target', 'redish'], cwd=build_dir, check=True)
    #
    #     redis_server_bin = os.path.join(build_dir, 'redish')
    #     cls._redis_proc = subprocess.Popen(
    #         [redis_server_bin],
    #         stdout=subprocess.PIPE,
    #         stderr=subprocess.PIPE
    #     )
    #     time.sleep(0.5)
    #
    #     cls.client = redis.Redis(host='127.0.0.1', port=6379)
    #     if not cls.client.ping():
    #         cls._redis_proc.terminate()
    #         cls._redis_proc.wait()
    #         raise RuntimeError("Could not start Redis server")

    # @classmethod
    # def tearDownClass(cls):
    #     cls._redis_proc.terminate()
    #     cls._redis_proc.wait()

    def setUp(self):
        self.connection = Connection()
        self.connection.connect()
        self.send("flushdb")

    def send(self, command, *args):
        self.connection.send_command(command, *args)
        response = self.connection.read_response()
        return response

    def test_unknown_command(self):
        with self.assertRaises(ResponseError):
            self.send("pinggggggggggg")

    def test_empty_command(self):
        with self.assertRaises(ResponseError):
            self.send("")

    def test_ping(self):
        self.assertEqual(b"PONG", self.send("ping"))

    def test_ping_arg(self):
        self.assertEqual(b"arg", self.send("ping", "arg"))

    def test_get_nonexistent(self):
        self.assertIsNone(self.send("get", "key"))

    def test_set_nonexistent(self):
        self.assertEqual(b"OK", self.send("set", "key", "value"))

    def test_get_existing(self):
        self.send("set", "key", "value")
        self.assertEqual(b"value", self.send("get", "key"))

    def test_set_existing(self):
        self.send("set", "key", "value")
        self.assertEqual(b"OK", self.send("set", "key", "value1"))
        self.assertEqual(b"value1", self.send("get", "key"))

    def test_set_xx(self):
        self.send("set", "key", "value")
        self.assertEqual(b"OK", self.send("set", "key", "value1"))

    def test_set_nx(self):
        self.assertEqual(b"OK", self.send("set", "key", "value"))

    def test_set_xx_conflict(self):
        self.assertIsNone(self.send("set", "key", "value", "xx"))

    def test_set_nx_conflict(self):
        self.send("set", "key", "value")
        self.assertIsNone(self.send("set", "key", "value1", "nx"))

    def test_set_and_get_nonexistent(self):
        self.assertIsNone(self.send("set", "key", "value", "get"))

    def test_set_and_get_existing(self):
        self.send("set", "key", "value")
        self.assertEqual(b"value", self.send("set", "key", "value1", "get"))
        self.assertEqual(b"value1", self.send("set", "key", "value2", "get"))

    def test_set_ex(self):
        self.send("set", "key", "value", "ex", "1")
        self.assertEqual(b"value", self.send("get", "key"))
        time.sleep(1.1)
        self.assertEqual(None, self.send("get", "key"))

    def test_set_px(self):
        self.send("set", "key", "value", "px", "1000")
        self.assertEqual(b"value", self.send("get", "key"))
        time.sleep(1.1)
        self.assertEqual(None, self.send("get", "key"))

    def test_set_exat(self):
        expiry = str(int(time.time()) + 1)
        self.send("set", "key", "value", "exat", expiry)
        self.assertEqual(b"value", self.send("get", "key"))
        time.sleep(1.1)
        self.assertEqual(None, self.send("get", "key"))

    def test_set_pxat(self):
        expiry = str(int((time.time() + 1) * 1000))
        self.send("set", "key", "value", "pxat", expiry)
        self.assertEqual(b"value", self.send("get", "key"))
        time.sleep(1.1)
        self.assertEqual(None, self.send("get", "key"))

    def test_exists_does_exist(self):
        self.send("set", "key", "value")
        self.assertEqual(1, self.send("exists", "key"))

    def test_exists_does_not_exist(self):
        self.send("set", "key", "value")
        self.assertEqual(0, self.send("exists", "key1"))

    def test_exists_multiple(self):
        self.send("set", "key", "value")
        self.send("set", "key1", "value")
        self.send("set", "key2", "value")
        self.assertEqual(2, self.send("exists", "key1", "key2"))


if __name__ == '__main__':
    unittest.main()
