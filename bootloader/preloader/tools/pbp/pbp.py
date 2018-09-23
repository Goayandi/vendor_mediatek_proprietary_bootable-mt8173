import sys
import os
import struct
import subprocess
import shutil
script_folder, script_name = os.path.split(os.path.realpath(__file__))
sys.path.append(os.path.join(script_folder, "lib"))
import gfh
import cert

def get_file_sizeb(file_path):
	if not os.path.isfile(file_path):
		return 0
	file_handle = open(file_path, "rb")
	file_handle.seek(0, 2)
	file_size = file_handle.tell()
	file_handle.close()
	return file_size

def concatb(file1_path, file2_path):
	file1_size = get_file_sizeb(file1_path)
	file2_size = get_file_sizeb(file2_path)
	file1 = open(file1_path, "ab+")
	file2 = open(file2_path, "rb")
	file1.write(file2.read(file2_size))
	file2.close()
	file1.close()

class bootloader:
	def __init__(self, out_path, in_bootloader_file_path, out_bootloader_file_path):
		self.m_gfh = gfh.image_gfh()
		self.m_out_path = out_path
		if not os.path.exists(self.m_out_path):
			os.makedirs(self.m_out_path)
		self.m_in_bootloader_file_path = in_bootloader_file_path
		self.m_out_bootloader_file_path = out_bootloader_file_path
		self.m_bootloader_is_signed = False
		self.m_bootloader_content_offset = 0
		#initialize content size to bootloader file size
		self.m_bootloader_content_length = get_file_sizeb(self.m_in_bootloader_file_path)
		self.m_bootloader_sig_size = 0
		#generate file path for bootloader without gfh and signature
		bootloader_path = in_bootloader_file_path.split('.')
		self.m_bootloader_no_gfh_file_path = bootloader_path[0] + "_plain.bin"
		self.m_sig_ver = 0
		self.m_sw_ver = 0
		self.m_root_prvk_path = ""
		self.m_img_prvk_path = ""
		self.m_ac_key = 0
		self.m_sig_handler = None
	def is_signed(self):
		if self.m_in_bootloader_file_path:
			bootloader_file = open(self.m_in_bootloader_file_path, "rb")
			gfh_hdr_obj = gfh.gfh_header()
			gfh_hdr_size = gfh_hdr_obj.get_size()
			gfh_hdr_buf = bootloader_file.read(gfh_hdr_size)
			self.m_bootloader_is_signed = gfh_hdr_obj.is_gfh(gfh_hdr_buf)
			bootloader_file.close()
		return self.m_bootloader_is_signed
	def parse(self):
		print "===parse bootloader==="
		#image will be decomposed if it's signed
		if self.is_signed():
			gfh_total_size = self.m_gfh.parse(self.m_in_bootloader_file_path)
			self.m_bootloader_content_offset = gfh_total_size
			self.m_bootloader_content_length -= gfh_total_size
			self.m_bootloader_content_length -= self.m_gfh.get_sig_size()
			self.m_bootloader_sig_size = self.m_gfh.get_sig_size()
			in_file = open(self.m_in_bootloader_file_path, "rb")
			out_file = open(self.m_bootloader_no_gfh_file_path, "wb")
			in_file.seek(self.m_bootloader_content_offset)
			out_file.write(in_file.read(self.m_bootloader_content_length))
			out_file.close()
			in_file.close()
		else:
			shutil.copyfile(self.m_in_bootloader_file_path, self.m_bootloader_no_gfh_file_path)
		print "bootloader content size = " + hex(self.m_bootloader_content_length)
	def create_gfh(self, gfh_config):
		self.parse()
		if gfh_config:
			if self.is_signed():
				del self.m_gfh.gfhs[:]
			self.m_gfh.load_ini(gfh_config)
		elif not self.is_signed():
			print "GFH_CONFIG.ini does not exist!!"
			return -1
		#self.m_gfh.dump()
		return 0
	def sign(self, key_ini_path, key_cert_path, content_config_file_path):
		self.m_gfh.finalize(self.m_bootloader_content_length, key_ini_path)
		#create tbs_bootloader.bin
		tbs_bootloader_file_path = os.path.join(self.m_out_path, "tbs_preloader.bin")
		tbs_bootloader_file = open(tbs_bootloader_file_path, "wb")
		tbs_bootloader_file.write(self.m_gfh.pack())
		bootloader_no_gfh_file = open(self.m_bootloader_no_gfh_file_path, "rb")
		tbs_bootloader_file.write(bootloader_no_gfh_file.read(self.m_bootloader_content_length))
		bootloader_no_gfh_file.close()
		tbs_bootloader_file.close()
		print "===sign==="
		if self.m_gfh.get_sig_type() == "cert_chain":
			self.m_sig_handler = cert.cert_chain_v2()
			#create key cert if key cert does not exist
			if key_cert_path == "":
				key_cert_path = os.path.join(self.m_out_path, "key_cert.bin")
			if not os.path.isfile(key_cert_path):
				key_cert_folder_name, key_cert_file_name = os.path.split(os.path.abspath(key_cert_path))
				self.m_sig_handler.create_key_cert(key_ini_path, self.m_out_path, key_cert_file_name)
				key_cert_path = os.path.join(self.m_out_path, key_cert_file_name)
			else:
				self.m_sig_handler.set_key_cert(key_cert_path)
			#create content cert
			content_cert_name = "content_cert.bin"
			self.m_sig_handler.create_content_cert(content_config_file_path, tbs_bootloader_file_path, self.m_out_path, content_cert_name)
			#create final cert chain
			sig_name = "preloader.sig"
			sig_file_path = os.path.join(self.m_out_path, sig_name)
			self.m_sig_handler.output(self.m_out_path, sig_name)
			#create final preloader image
			if os.path.isfile(self.m_out_bootloader_file_path):
				os.remove(self.m_out_bootloader_file_path)
			concatb(self.m_out_bootloader_file_path, tbs_bootloader_file_path)
			concatb(self.m_out_bootloader_file_path, sig_file_path)
			#clean up
			os.remove(os.path.join(self.m_out_path, content_cert_name))
		elif self.m_gfh.get_sig_type() == "single_and_phash":
			self.m_sig_handler = cert.sig_single_and_phash(self.m_gfh.get_pad_type())
			self.m_sig_handler.set_out_path(self.m_out_path)
			self.m_sig_handler.create(key_ini_path, tbs_bootloader_file_path)
			self.m_sig_handler.sign()
			sig_name = "preloader.sig"
			sig_file_path = os.path.join(self.m_out_path, sig_name)
			self.m_sig_handler.output(self.m_out_path, sig_name)
			#create final preloader image
			if os.path.isfile(self.m_out_bootloader_file_path):
				os.remove(self.m_out_bootloader_file_path)
			concatb(self.m_out_bootloader_file_path, tbs_bootloader_file_path)
			concatb(self.m_out_bootloader_file_path, sig_file_path)
		else:
			print "unknown signature type"
		#clean up
		os.remove(self.m_bootloader_no_gfh_file_path)
		os.remove(tbs_bootloader_file_path)
		os.remove(sig_file_path)
		return

def main():
	#parameter parsing
	idx = 1
	key_ini_path = ""
	key_path = ""
	gfh_config_ini_path = ""
	content_config_ini_path = ""
	key_cert_path = ""
	in_bootloader_path = ""
	function = "sign"
	function_out_path = ""
	while idx < len(sys.argv):
		if sys.argv[idx][0] == '-':
			if sys.argv[idx][1] == 'i':
				print "key ini: " + sys.argv[idx + 1]
				key_ini_path = sys.argv[idx + 1]
				idx += 2
			if sys.argv[idx][1] == 'j':
				print "key(pem): " + sys.argv[idx + 1]
				key_path = sys.argv[idx + 1]
				idx += 2
			elif sys.argv[idx][1] == 'g':
				print "gfh config: " + sys.argv[idx + 1]
				gfh_config_ini_path = sys.argv[idx + 1]
				idx += 2
			elif sys.argv[idx][1] == 'c':
				print "content config: " + sys.argv[idx + 1]
				content_config_ini_path = sys.argv[idx + 1]
				idx += 2
			elif sys.argv[idx][1] == 'k':
				print "key cert: " + sys.argv[idx + 1]
				key_cert_path = sys.argv[idx + 1]
				idx += 2
			elif sys.argv[idx][1:] == 'func':
				print "function: " + sys.argv[idx + 1]
				function = sys.argv[idx + 1]
				idx += 2
			elif sys.argv[idx][1] == 'o':
				print "function output: " + sys.argv[idx + 1]
				function_out_path = sys.argv[idx + 1]
				idx += 2
			else:
				print "unknown input"
				idx += 2
		else:
			in_bootloader_path = sys.argv[idx]
			print "bootloader: " + in_bootloader_path
			idx += 1

	if (function == "sign") and (not in_bootloader_path):
		print "bootloader path is not given!"
		return -1
	if (not function_out_path):
		print "function out path is not given!"
		return -1
	if function == "sign":
		if (key_ini_path == "") and (key_cert_path == ""):
			print "key path is not given!"
			return -1
	else:
		if (key_path == "" and key_ini_path == ""):
			print "key path is not given!"
			return -1

	out_path = os.path.dirname(os.path.abspath(function_out_path))
	if not os.path.exists(out_path):
		os.makedirs(out_path)

	if function == "sign":
		bootloader_obj = bootloader(out_path, in_bootloader_path, function_out_path)
		bootloader_obj.create_gfh(gfh_config_ini_path)
		bootloader_obj.sign(key_ini_path, key_cert_path, content_config_ini_path)
		return 0
	elif function == "keybin_pss":
		key = cert.ct_key("pss")
		key.create(key_path)
		key_bin = key.pack()
		out_file = open(function_out_path, "wb")
		out_file.write(key_bin)
		out_file.close()
		return 0
	elif function == "keybin_m1":
		key = cert.ct_key("m1")
		key.create(key_path)
		key_bin = key.pack()
		out_file = open(function_out_path, "wb")
		out_file.write(key_bin)
		out_file.close()
		return 0
	elif function == "keyhash_pss":
		key = cert.ct_key("pss")
		key.create(key_path)
		key_bin = key.pack()
		tmp_key_bin_path = os.path.join(out_path, "tmp_keybin.bin")
		out_file = open(tmp_key_bin_path, "wb")
		out_file.write(key_bin)
		out_file.close()
		cert.hash_gen(tmp_key_bin_path, function_out_path)
		os.remove(tmp_key_bin_path)
		return 0
	elif function == "keyhash_m1":
		key = cert.ct_key("m1")
		key.create(key_path)
		key_bin = key.pack()
		tmp_key_bin_path = os.path.join(out_path, "tmp_keybin.bin")
		out_file = open(tmp_key_bin_path, "wb")
		out_file.write(key_bin)
		out_file.close()
		cert.hash_gen(tmp_key_bin_path, function_out_path)
		os.remove(tmp_key_bin_path)
		return 0

if __name__ == '__main__':
	main()
