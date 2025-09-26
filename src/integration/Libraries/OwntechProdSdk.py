import requests
import json

class OwntechProdSdk:
	def __init__(self, server_address): 
		self.server_address = server_address
		self.token = ""

	def register(self, user, password):
		register_req = requests.post(self.server_address + "auth/register", data={"username": user, "password" : password})
		try:	
			res_json = register_req.json()
			return {'status_code': register_req.status_code, 'content': res_json}
		except:
			print("JSON not valid during register")
			print(register_req.text)
			exit(1)


	def login(self, user, password):
		login_req = requests.post(self.server_address + "auth/login", data={"username": user, "password" : password})
		try:	
			res_json = login_req.json()
			self.token = res_json["token"]
			return {'status_code': login_req.status_code, 'content': res_json}
		except:
			print("OwnTech Api's error")
			print(login_req.text)
			exit(1)


	def generate_serial(self, product_type, product_verison):
		auth_header = {"Authorization": f"Bearer {self.token}" }
		serial_req = requests.post(self.server_address + "test/generateSerial", headers=auth_header, data={"type": product_type, "version" : product_verison})
		try:
			res_json = serial_req.json()
			return {'status_code': serial_req.status_code, 'content': res_json}
		except:
			print("OwnTech Api's error")
			print(serial_req.text)
			exit(1)

	def add_test_report(self, serial, report):
		auth_header = {"Authorization": f"Bearer {self.token}"}
		report_req = requests.post(self.server_address + "test/addTestReport", headers=auth_header, data={ "report": report, "serial": serial })
		try:	
			res_json = report_req.json()
			return {'status_code': report_req.status_code, 'content': res_json}
		except:
			print("OwnTech Api's error")
			print(report_req.text)
			exit(1)

	def get_test_report(self, serial):
		auth_header = {"Authorization": f"Bearer {self.token}"}
		report_req = requests.get(self.server_address + "test/getTestReport/" + serial , headers=auth_header)
		try:	
			res_json = report_req.json()
			print(report_req.url)
			return {'status_code': report_req.status_code, 'content': res_json}
		except:
			print("Response is not a json")
			print(report_req.text)
			exit(1)

