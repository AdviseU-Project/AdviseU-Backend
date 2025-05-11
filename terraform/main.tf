provider "aws" {
  region = "us-east-2"
}

# ECR Repository for your Docker images
resource "aws_ecr_repository" "app" {
  name = "adviseu-backend"
}

# ECS Cluster
resource "aws_ecs_cluster" "main" {
  name = "adviseu-cluster"
}

# IAM role for ECS tasks
resource "aws_iam_role" "ecs_task_execution_role" {
  name = "adviseu-execution-role"
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action = "sts:AssumeRole"
      Effect = "Allow"
      Principal = {
        Service = "ecs-tasks.amazonaws.com"
      }
    }]
  })
}

# Attach the ECS task execution policy
resource "aws_iam_role_policy_attachment" "ecs_task_execution_role_policy" {
  role       = aws_iam_role.ecs_task_execution_role.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AmazonECSTaskExecutionRolePolicy"
}

# CloudWatch log group
resource "aws_cloudwatch_log_group" "app_logs" {
  name = "/ecs/adviseu-backend"
  retention_in_days = 7
}

# ECS Task Definition
resource "aws_ecs_task_definition" "app" {
  family                   = "adviseu-backend"
  network_mode             = "awsvpc"
  requires_compatibilities = ["FARGATE"]
  cpu                      = "1024" # 1 vCPU
  memory                   = "2048" # 2 GB RAM
  execution_role_arn       = aws_iam_role.ecs_task_execution_role.arn

  container_definitions = jsonencode([{
    name        = "adviseu-backend"
    image       = "${aws_ecr_repository.app.repository_url}:latest"
    essential   = true

    portMappings = [{
      containerPort = 8080
      hostPort      = 8080
    }]

    environment = [
      { name = "CONTAINER_PORT", value = "8080" },
      { name = "MONGO_URI", value = var.mongo_uri }
    ]

    healthCheck = {
      command     = ["CMD-SHELL", "curl -f http://localhost:8080/health || exit 1"]
      interval    = 30
      timeout     = 5
      retries     = 3
      startPeriod = 60
    }

    logConfiguration = {
      logDriver = "awslogs"
      options = {
        "awslogs-group"         = aws_cloudwatch_log_group.app_logs.name
        "awslogs-region"        = "us-east-2"
        "awslogs-stream-prefix" = "ecs"
      }
    }
  }])
}

# Use the default VPC and subnets
data "aws_vpc" "default" {
  default = true
}

data "aws_subnets" "default" {
  filter {
    name   = "vpc-id"
    values = [data.aws_vpc.default.id]
  }
}

# Security group for the ALB
resource "aws_security_group" "alb" {
  name_prefix = "adviseu-alb-sg-"
  vpc_id      = data.aws_vpc.default.id

  ingress {
    from_port   = 80
    to_port     = 80
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
    description = "Allow HTTP traffic"
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

# Security group for the ECS tasks
resource "aws_security_group" "ecs_tasks" {
  name_prefix = "adviseu-ecs-sg-"
  vpc_id      = data.aws_vpc.default.id

  ingress {
    from_port   = 8080
    to_port     = 8080
    protocol    = "tcp"
    security_groups = [aws_security_group.alb.id]
    description = "Allow traffic from the ALB"
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

# Create an Application Load Balancer
resource "aws_lb" "app_lb" {
  name               = "adviseu-lb"
  internal           = false
  load_balancer_type = "application"
  security_groups    = [aws_security_group.alb.id]
  subnets            = data.aws_subnets.default.ids
}

# Create a target group for your ECS service
resource "aws_lb_target_group" "app_tg" {
  name     = "adviseu-tg"
  port     = 8080
  protocol = "HTTP"
  vpc_id   = data.aws_vpc.default.id
  target_type = "ip"

  health_check {
    path     = "/health"
    protocol = "HTTP"
    matcher  = "200"
    interval = 30
    timeout  = 5
    healthy_threshold   = 2
    unhealthy_threshold = 2
  }
}

# Create a listener for the ALB to forward traffic to the target group
resource "aws_lb_listener" "http" {
  load_balancer_arn = aws_lb.app_lb.arn
  port              = "80"
  protocol          = "HTTP"

  default_action {
    target_group_arn = aws_lb_target_group.app_tg.arn
    type             = "forward"
  }
}

# ECS Service with Load Balancer Integration
resource "aws_ecs_service" "app" {
  name            = "adviseu-service"
  cluster         = aws_ecs_cluster.main.id
  task_definition = aws_ecs_task_definition.app.arn
  desired_count   = 1
  launch_type     = "FARGATE"

  network_configuration {
    subnets          = data.aws_subnets.default.ids
    security_groups  = [aws_security_group.ecs_tasks.id]
    assign_public_ip = true
  }

  load_balancer {
    target_group_arn = aws_lb_target_group.app_tg.arn
    container_name   = "adviseu-backend"
    container_port   = 8080
  }
}

# Variables
variable "mongo_uri" {
  description = "MongoDB connection string"
  type        = string
  sensitive   = true
}

# Outputs
output "ecr_repository_url" {
  value = aws_ecr_repository.app.repository_url
}

output "load_balancer_url" {
  value = aws_lb.app_lb.dns_name
}
