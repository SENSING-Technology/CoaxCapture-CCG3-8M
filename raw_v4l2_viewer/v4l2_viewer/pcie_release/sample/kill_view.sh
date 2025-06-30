# 使用pgrep命令查找进程ID
pids=$(pgrep view)

# 检查是否找到了进程
if [ -z "$pids" ]; then
  echo "未找到与 $process_name 相关的进程"
else
  # 循环杀死进程
  for pid in $pids; do
    kill -9 $pid
    if [ $? -eq 0 ]; then
      echo "进程 $pid 已成功终止"
    else
      echo "无法终止进程 $pid"
    fi
  done
fi
