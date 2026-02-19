import tomli_w, random

data = {}
hardware = {
    "Processors / CPU": [
        "register",
        "cache",
        "pipeline",
        "instruction",
        "opcode",
        "core",
        "thread",
        "microcode",
        "ALU",
        "FPU"
    ],
    "Memory": [
        "DRAM",
        "SRAM",
        "EEPROM",
        "flash",
        "DIMM",
        "address",
        "page",
        "latency",
        "bandwidth",
        "refresh"
    ],
    "Storage": [
        "sector",
        "platter",
        "spindle",
        "NAND",
        "block",
        "cluster",
        "partition",
        "filesystem",
        "IOPS",
        "controller"
    ],
    "I/O & Buses": [
        "PCIe",
        "USB",
        "SATA",
        "Thunderbolt",
        "bus",
        "lane",
        "DMA",
        "interrupt",
        "channel",
        "transceiver"
    ],
    "Power & Electronics": [
        "transistor",
        "diode",
        "capacitor",
        "inductor",
        "resistor",
        "MOSFET",
        "voltage regulator",
        "clock",
        "oscillator",
        "power supply"
    ],
    "Networking Hardware": [
        "router",
        "switch",
        "NIC",
        "PHY",
        "MAC",
        "antenna",
        "port",
        "cable",
        "SFP",
        "backplane"
    ],
    "Graphics / Accelerators": [
        "shader",
        "rasterizer",
        "framebuffer",
        "GPU",
        "VRAM",
        "texture",
        "pipeline",
        "multiprocessor",
        "tensor core",
        "ray tracing unit"
    ]
}

descriptors = [
    "ClockSpeed",
    "CycleTime",
    "Latency",
    "Throughput",
    "Bandwidth",
    "BusWidth",
    "WordSize",
    "CacheSize",
    "CacheLineLength",
    "PipelineDepth",
    "InstructionSetWidth",
    "RegisterCount",
    "ThermalDesignPower",
    "PowerDraw",
    "Voltage",
    "Current",
    "Resistance",
    "Capacitance",
    "Inductance",
    "Impedance",
    "SignalIntegrity",
    "Jitter",
    "PropagationDelay",
    "SwitchingSpeed",
    "RiseTime",
    "FallTime",
    "NoiseMargin",
    "ErrorRate",
    "FaultTolerance",
    "Redundancy",
    "Reliability",
    "Availability",
    "Uptime",
    "MeanTimeBetweenFailures",
    "MeanTimeToRepair",
    "Scalability",
    "Parallelism",
    "Concurrency",
    "Utilization",
    "Efficiency",
    "PerformancePerWatt",
    "HeatDissipation",
    "ThermalHeadroom",
    "CoolingCapacity",
    "FanSpeed",
    "Airflow",
    "VibrationTolerance",
    "ShockResistance",
    "FormFactor",
    "DieSize",
    "TransistorCount",
    "ProcessNode",
    "FabricationYield",
    "InterconnectDensity",
    "SignalToNoiseRatio",
    "DutyCycle",
    "PhaseShift",
    "FrequencyResponse",
    "HarmonicDistortion",
    "ElectromagneticInterference",
    "Crosstalk",
    "Stability",
    "Drift",
    "Aging",
    "WearOut",
    "Endurance",
    "RetentionTime",
    "RefreshInterval",
    "DataIntegrity",
    "ChecksumRate",
    "ParityCoverage",
    "ErrorCorrectionOverhead",
    "ThroughputPerLane",
    "LatencyPerHop",
    "QueueDepth",
    "BufferSize",
    "BurstLength",
    "AccessTime",
    "SeekTime",
    "IOPS",
    "TransferRate",
    "SustainedRate",
    "PeakRate",
    "IdlePower",
    "ActivePower",
    "SleepPower",
    "BootTime",
    "ResetTime",
    "InitializationOverhead",
    "HandshakeDelay",
    "NegotiationSpeed"
]

writerIds = set()

def generateData(pubCount: int, groupCount: int, dataSetCount: int, fieldCount: int) -> dict:
    items = list(hardware.items())
    data = {"publishers" : []}
    for publisher in range(pubCount):
        pubId = (publisher + 1) * 10000
        indecesGroup = random.sample(range(len(items)), groupCount)
        # print("indecesGroup", indecesGroup)
        writerGroups = [] 
        
        for indexGroup in indecesGroup:
            indecesDataSet = random.sample(range(len(items[indexGroup][1])), dataSetCount)
            groupId = indexGroup * 100 + 100
            # print("indecesDataSet", indecesDataSet)
            dataSets = []
            for indexDataSet in indecesDataSet:
                indecesFields = random.sample(range(len(descriptors)), fieldCount)
                fields = [{"name" : items[indexGroup][1][indexDataSet] + descriptors[indexField], "type" : "Float"} for indexField in indecesFields]
                writerId = pubId + groupId  + indexDataSet
                if writerId in writerIds:
                    print("writerId", writerId)
                writerIds.add(writerId)
                dataSets.append({"writerId" : writerId, "name" : f"{items[indexGroup][1][indexDataSet]}_{writerId}" , "fields" : fields})
            writerGroups.append({"id" : groupId, "interval" : 100, "name" : items[indexGroup][0], "dataSets" : dataSets})
        data["publishers"].append({"name" : "PLC" + str(publisher), "publisherId" : pubId, "writerGroups" : writerGroups})
    return data
        
if __name__ == "__main__":
    with open("config.toml", "wb") as f:
        tomli_w.dump(generateData(2, 5, 10, 2), f)
        
    
        