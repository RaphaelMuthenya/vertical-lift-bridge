import pandas as pd
class Machine:
    def __init__(self, name, cost_per_hour):
        self.name = name
        self.cost_per_hour = cost_per_hour

    def calculate_cost(self, hours):
        return self.cost_per_hour * hours

    def __str__(self):
        return f"Machine: {self.name}, Cost per hour: {self.cost_per_hour}"
# Example usage
machine1 = Machine("Machine A", 10)
machine2 = Machine("Machine B", 15)
machine3 = Machine("Machine C", 20)

print(machine1)
print(machine2)
print(machine3)